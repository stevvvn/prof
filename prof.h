#ifndef PROF_H
#define PROF_H

#ifdef PROFILE
#define PROF_ENTER(name, args...) Prof::ScopeCanary _PROF_CAN = Prof::Engine::enter(#name, __FILE__, __LINE__ , ## args);
#define PROF_REPORT() Prof::Engine::report();
#define PROF_SUMMARY_REPORT() Prof::Engine::summaryReport();
#else
#define PROF_ENTER(name, ...)
#define PROF_REPORT()
#define PROF_SUMMARY_REPORT()
#endif

#include <string>
#include <map>
#include <vector>
#include <sstream>
#include <boost/shared_ptr.hpp>
#include <sys/time.h>
#ifdef __MACH__
	#include <mach/mach_time.h>
	#define ORWL_NANO (+1.0E-9)
	#define ORWL_GIGA UINT64_C(1000000000)
#endif

namespace Prof
{

class Context
{
public:
	/**
	 * Start recording a block of time
	 * @param name name of the function, or whatever arbitrary identifier you'd
	 *             like, really
	 * @param file filename, added automatically when invoked through the macro
	 * @param line line of the file, added automatically when invoked through 
	 *             the macro
	 * @param root whether this is an outermost part of the context hierarchy
	 * @param args any additional parameters you want to record. currently these
	 *             show up on full report, but they might be more useful later. 
	 *             probably not if I'm honest
	 * @return Token that should be passed to the corresponding exit() call
	 */
	template <typename ... Args>
	Context(const std::string& name, const std::string& file, const size_t line, bool root, const Args&... args) : 
	name(name), file(file), line(line), ts(), args(), orwl_timestart(0), orwl_timebase(0.0), children(), root(root), ended(false) {
		std::stringstream ss;
		stringify(ss, args...);
		ts.tv_sec = 0;
		ts.tv_nsec = 0;
#if defined CLOCK_MONOTONIC_RAW 
		clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
#elif defined CLOCK_MONOTONIC
		clock_gettime(CLOCK_MONOTONIC, &ts);
#elif defined __MACH__
		mach_timebase_info_data_t tb;
		mach_timebase_info(&tb);
		orwl_timebase = tb.numer;
		orwl_timebase /= tb.denom;
		orwl_timestart = mach_absolute_time();
#else
		#error "not sure how to get monotonic time on this platform"
#endif
	}

	/**
	 * Compare current time to stored start time
	 */
	void markEnd();

	/**
	 * Indicate that another context was created while this one was active
	 * @param ctx child context
	 */
	void pushChild(const boost::shared_ptr<Context> ctx);

	/**
	 * @return number of children registered
	 */
	size_t getChildCount() const;

	/**
	 * @return arbitrary identifier, likely a function name
	 */
	const std::string& getName() const;

	/**
	 * @return filename where context was started
	 */
	const std::string& getFile() const;
	
	/** 
	 * @return file line where context was started
	 */
	size_t getLine() const;
	
	/**
	 * @return timespec representing length of time the context lasted
	 */
	const timespec& getTimeSpec() const;

	/**
	 * @return whether this is an outermost node in the context hierarchy
	 */ 
	bool isRoot() const;

	/**
	 * @return string representatin of arguments
	 */
	std::string getArgumentString() const;

	/**
	 * @return immediately-nested contexts
	 */
	const std::vector<boost::shared_ptr<Context> >& getChildren() const;
private:
	/**
	 * Convert varargs to vector of string representations
	 * @param ss stringstream to be used to convert
	 * @param val currently-considered value
	 * @param args further vals
	 */
	template <typename T, typename ... Args>
	void stringify(std::stringstream& ss, const T& val, const Args&... args) {
		ss << val;
		this->args.push_back(ss.str());
		ss.str("");
		stringify(ss, args...);
	}

	/**
	 * Stop point for recursive stringify calls when there are no more varargs
	 */
	void stringify(std::stringstream&) {}

	std::string name;
	std::string file;
	size_t line;
	timespec ts;
	std::vector<std::string> args;
	uint64_t orwl_timestart;
	double orwl_timebase;
	std::vector<boost::shared_ptr<Context> > children;
	bool root, ended;
};

class ScopeCanary
{
public:
	ScopeCanary(boost::shared_ptr<Context> ctx);
	~ScopeCanary();
private:
	boost::shared_ptr<Context> ctx;
};

class Engine
{
public:
	typedef size_t Token;

	/**
	 * Start recording a block of time
	 * @param name name of the function, or whatever arbitrary identifier you'd
	 *             like, really
	 * @param file filename, added automatically when invoked through the macro
	 * @param line line of the file, added automatically when invoked through 
	 *             the macro
	 * @param args any additional parameters you want to record. currently these
	 *             show up on full report, but they might be more useful later. 
	 *             probably not if I'm honest
	 * @return Token that should be passed to the corresponding exit() call
	 */
	template <typename ... Args>
	static ScopeCanary enter(const std::string& name, const std::string& file, size_t line, const Args&... args) {
		bool isRoot = active.empty();
		boost::shared_ptr<Context> ctx(new Context(name, file, line, isRoot, args...));
		if (!isRoot) {
			active.back()->pushChild(ctx);
		}
		active.push_back(ctx);
		records.push_back(ctx);
		return ScopeCanary(ctx);
	}

	static void exit();

	/**
	 * Output a list of functions ordered by the total time they took
	 */
	static void summaryReport();
		
	/**
	 * Print a full report of everything that was logged
	 */
	static void report();
private:
	/**
	 * Output a context entry
	 * @param ctx Context
	 * @param depth number of callers nested above this context
	 */ 
	static void stringify(boost::shared_ptr<Context> ctx, size_t depth);

	/** 
	 * Format the seconds and nanoseconds fields of a timespec as a string
	 * @param ts timespec
	 */
	static std::string timeSpecToString(timespec ts);

	/**
	 * Fill a map of filename:function keys to values indicating the time the 
	 * indicated function took, taking care not to double-count recursive calls
	 * @param times map of filename:function->timespec
	 * @param used map indicating which keys should be considered recursive calls
	 * @param ctx currently-considered bit of context
	 */
	static void totalTime(std::map<std::string, timespec>& times, std::map<std::string, bool> used, const boost::shared_ptr<Context> ctx);

	static std::vector<boost::shared_ptr<Context> > active;
	static std::vector<boost::shared_ptr<Context> > records;
};


}

#endif

