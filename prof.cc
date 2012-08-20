#ifndef PROF_CC
#define PROF_CC

#include "prof.h"

#include <iostream>
#include <cstdarg>
#include <sstream>
#include <boost/format.hpp>

namespace Prof 
{

ScopeCanary::ScopeCanary(boost::shared_ptr<Context> ctx) : ctx(ctx) {
}

ScopeCanary::~ScopeCanary() {
	Engine::exit();
	ctx->markEnd();
}

void Context::markEnd() {
	if (!ended) {
		ended = true;
#if defined CLOCK_MONOTONIC_RAW || defined CLOCK_MONOTONIC
		timespec after;
#ifdef CLOCK_MONOTONIC_RAW
		clock_gettime(CLOCK_MONOTONIC_RAW, &after);
#else
		clock_gettime(CLOCK_MONOTONIC, &after);
#endif
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

void Engine::exit() {
	active.pop_back();
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
	for (std::vector<boost::shared_ptr<Context> >::const_iterator ci = records.begin(); ci != records.end(); ++ci) {
		if ((*ci)->isRoot()) {
			(*ci)->markEnd();
			stringify(*ci, 0);
		}
	}
}

std::string Engine::timeSpecToString(timespec ts) {
	return (boost::format("%lld.%09ld") % (long long)ts.tv_sec % ts.tv_nsec).str();
}

void Engine::totalTime(std::map<std::string, timespec>& times, std::map<std::string, bool> used, const boost::shared_ptr<Context> ctx) {
	std::stringstream ss;
	ss << ctx->getFile() << ":" << ctx->getLine() << " - " << ctx->getName();
	std::string key = ss.str();
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
	for (std::vector<boost::shared_ptr<Context> >::const_iterator ci = records.begin(); ci != records.end(); ++ci) {
		if ((*ci)->isRoot()) {
			(*ci)->markEnd();
			totalTime(times, std::map<std::string, bool>(), *ci);
		}
	}
	std::vector<std::pair<std::string, timespec> > sortedTimes;
	for (std::map<std::string, timespec>::const_iterator ci = times.begin(); ci != times.end(); ++ci) {
		sortedTimes.push_back(*ci);
	}
	std::sort(sortedTimes.begin(), sortedTimes.end(), [](const std::pair<std::string, timespec>& a, const std::pair<std::string, timespec>& b) -> bool {
		return a.second.tv_sec > b.second.tv_sec || (a.second.tv_sec == b.second.tv_sec && a.second.tv_nsec > b.second.tv_nsec);
	});
	for (std::vector<std::pair<std::string, timespec> >::const_iterator ci = sortedTimes.begin(); ci != sortedTimes.end(); ++ci) {
		std::cout << ci->first << " - " << timeSpecToString(ci->second) << "\n";
	}
}

std::vector<boost::shared_ptr<Context> > Engine::records;
std::vector<boost::shared_ptr<Context> > Engine::active;

}
#endif
