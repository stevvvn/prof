#include "prof.h"

#include <iostream>
#include <cstdarg>
#include <sstream>
#include <boost/format.hpp>

#ifdef __MACH__
	#include <mach/mach_time.h>
	#define ORWL_NANO (+1.0E-9)
	#define ORWL_GIGA UINT64_C(1000000000)
#endif

namespace Prof 
{

template <typename ... Args>
Context::Context(const std::string& name, const std::string& file, const size_t line, bool root, const Args&... args) : 
	name(name), file(file), line(line), ts(), args(), orwl_timestart(0), orwl_timebase(0.0), children(), root(root), clockType(0) {
	std::stringstream ss;
	stringify(ss, args...);
	ts.tv_sec = 0;
	ts.tv_nsec = 0;
#ifdef CLOCK_MONOTONIC_RAW
	clockType = CLOCK_MONOTONIC_RAW;
	clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#elif CLOCK_MONOTONIC
	clockType = CLOCK_MONOTONIC;
	clock_gettime(CLOCK_MONOTONIC, &ts);
#elif __MACH__
	mach_timebase_info_data_t tb;
	mach_timebase_info(&tb);
	orwl_timebase = tb.numer;
	orwl_timebase /= tb.denom;
	orwl_timestart = mach_absolute_time();
#else
	#error "not sure how to get monotonic time on this platform"
#endif
}

template <typename T, typename ... Args>
void Context::stringify(std::stringstream& ss, const T& val, const Args&... args) {
	ss << val;
	this->args.push_back(ss.str());
	ss.str("");
	stringify(ss, args...);
}

void Context::stringify(std::stringstream&) {
}

void Context::markEnd() {
#if defined CLOCK_MONOTONIC_RAW || defined CLOCK_MONOTONIC
	timespec after;
	clock_gettime(clockType, &after);
	ts.tv_sec = after.tv_sec - ts.tv_sec;
	ts.tv_nsec = after.tv_nsec - ts.tv_nsec;
	if (ts.tv_nsec < 0) {
		--ts.tv_sec;
		ts.tv_nsec = 1000000000 + ts.tv_nsec;
	}
#elif __MACH__
	double diff = (mach_absolute_time() - orwl_timestart) * orwl_timebase;
	ts.tv_sec = diff * ORWL_NANO;
	ts.tv_nsec = diff - (ts.tv_sec * ORWL_GIGA);
#endif
}

void Context::pushChild(const boost::shared_ptr<Context> ctx) {
	children.push_back(ctx);
}

size_t Context::getChildCount() const {
	return children.size();
}

const std::string& Context::getName() const {
	return name;
}

const std::string& Context::getFile() const {
	return file;
}
	
size_t Context::getLine() const {
	return line;
}
	
const timespec& Context::getTimeSpec() const {
	return ts;
}

bool Context::isRoot() const {
	return root;
}

std::string Context::getArgumentString() const {
	std::stringstream ss;
	for (size_t idx = 0; idx < args.size(); ++idx) {
		ss << (idx == 0 ? "" : ", ") << args[idx];
	}
	return ss.str();
}

const std::vector<boost::shared_ptr<Context> >& Context::getChildren() const {
	return children;
}

template <typename ... Args>
Engine::Token Engine::enter(const std::string& name, const std::string& file, size_t line, const Args&... args) {
	++calls;
	bool isRoot = active.empty();
	boost::shared_ptr<Context> ctx(new Context(name, file, line, isRoot, args...));
	if (!isRoot) {
		active.back()->pushChild(ctx);
	}
	active.push_back(ctx);
	records[calls] = ctx;
	return calls;
}

void Engine::exit(Engine::Token t) {
	active.pop_back();
	records[t]->markEnd();
}

void Engine::stringify(boost::shared_ptr<Context> ctx, size_t depth) {
	std::cout << std::string(depth, '\t') << ctx->getFile() << ":" << ctx->getLine() << " - " 
		<< ctx->getName() << "(" << ctx->getArgumentString() << ") - " 
		<< timeSpecToString(ctx->getTimeSpec()) << "\n";
	const std::vector<boost::shared_ptr<Context> >& children = ctx->getChildren();	
	for (std::vector<boost::shared_ptr<Context> >::const_iterator ci = children.begin(); ci != children.end(); ++ci) {
		stringify(*ci, depth + 1);
	}
}

void Engine::report() {
	for (std::map<Token, boost::shared_ptr<Context> >::const_iterator ci = records.begin(); ci != records.end(); ++ci) {
		if (ci->second->isRoot()) {
			stringify(ci->second, 0);
		}
	}
}

std::string Engine::timeSpecToString(timespec ts) {
	return (boost::format("%lld.%09ld") % (long long)ts.tv_sec % ts.tv_nsec).str();
}

void Engine::totalTime(std::map<std::string, timespec>& times, std::map<std::string, bool> used, const boost::shared_ptr<Context> ctx) {
	std::string key = ctx->getFile() + ":" + ctx->getName();
	// recursive calls should not add time to outer call time
	if (!used[key]) {
		timespec ex = times[key];
		ex.tv_sec += ctx->getTimeSpec().tv_sec;
		ex.tv_nsec += ctx->getTimeSpec().tv_nsec;
		if (ex.tv_nsec > 1000000000) {
			ex.tv_nsec -= 1000000000;
			++ex.tv_sec;
		}			
		times[key] = ex;
	}

	if (ctx->getChildCount() > 0) {
		used[key] = true;
		const std::vector<boost::shared_ptr<Context> >& children = ctx->getChildren();	
		for (std::vector<boost::shared_ptr<Context> >::const_iterator ci = children.begin(); ci != children.end(); ++ci) {
			totalTime(times, used, *ci);
		}
	}
}

void Engine::summaryReport() {
	std::map<std::string, timespec> times;
	for (std::map<Token, boost::shared_ptr<Context> >::const_iterator ci = records.begin(); ci != records.end(); ++ci) {
		if (ci->second->isRoot()) {
			totalTime(times, std::map<std::string, bool>(), ci->second);
		}
	}
	std::vector<std::pair<std::string, timespec> > sortedTimes;
	for (std::map<std::string, timespec>::const_iterator ci = times.begin(); ci != times.end(); ++ci) {
		sortedTimes.push_back(*ci);
	}
	std::sort(sortedTimes.begin(), sortedTimes.end(), [](const std::pair<std::string, timespec>& a, const std::pair<std::string, timespec>& b) -> bool {
		if (a.second.tv_sec > b.second.tv_sec) {
			return true;
		}
		if (a.second.tv_sec < b.second.tv_sec) {
			return false;
		}
		if (a.second.tv_nsec > b.second.tv_nsec) {
			return true;
		}
		return false;
	});
	for (std::vector<std::pair<std::string, timespec> >::const_iterator ci = sortedTimes.begin(); ci != sortedTimes.end(); ++ci) {
		std::cout << ci->first << " - " << timeSpecToString(ci->second) << "\n";
	}
}

Engine::Token Engine::calls = 0;
size_t Engine::depth = 0;
std::map<Engine::Token, boost::shared_ptr<Context> > Engine::records;
std::vector<boost::shared_ptr<Context> > Engine::active;

}
