#include <unistd.h>

#include "scheduler.h"

using std::string;
using std::regex;
using std::smatch;
using std::map;

// wrappers
void Juggler::schedLaunchA(void* data) {
	((Juggler::Scheduler*) data)->schedClock();
}

void Juggler::schedLaunchB(void* data) {
	((Juggler::Scheduler*) data)->schedWaitLoop();
}

Juggler::Scheduler::Scheduler() {
	running = true;
	setupDateInfo();
	initLock(mapAccess);
	initCondition(eventToSchedule);
	m = new Juggler::ThreadManager(2);

	std::cout << "START AT: " << timeToString(time(NULL)) << "\n";

	m->lease(schedLaunchA, (void*) this, "scheduler clock that goes off every minute");
	m->lease(schedLaunchB, (void*) this, "scheduler wait loop that runs the commands set off by the clock");
}

string Juggler::Scheduler::upNext() {
	auto iter = events.begin();
	if(iter != events.end()) {
		return iter->second->des + string(" at ") + timeToString(iter->first);
	}
	else {
		return "No events scheduled";
	}
}

void Juggler::Scheduler::setupDateInfo() {
	time_t now;
	struct tm* d;
	time(&now);
	d = gmtime(&now);

	time_t l = timelocal(d);
	time_t g = timegm(d);

	d = localtime(&now); // to get DST
	daylightSavings = d->tm_isdst ? true : false;
	myTimeZone = ((g - l) / 3600);
	if(daylightSavings) {
		myTimeZone -= 1;
	}
}

int Juggler::Scheduler::dayCharToInt(char c) {
	switch(c) {
		case 'U':
			return 0;
		case 'M':
			return 1;
		case 'T':
			return 2;
		case 'W':
			return 3;
		case 'R':
			return 4;
		case 'F':
			return 5;
		case 'S':
			return 6;

		default:
			return -1; // error
	}
}

void Juggler::Scheduler::schedWaitLoop() {
	time_t now;
	bool scanAgain = true;
	TimeEvent* tmpEvt = NULL;
	
	while(running) {
		acquireLock(mapAccess);

		// block until we have at least one event
		waitOnCondition(eventToSchedule, mapAccess);
		time(&now);

		while(scanAgain) {
			for(auto iter = events.begin(); iter != events.end(); ++iter) {
				if(iter->first <= now) {
					iter->second->caller(); // make the call
					tmpEvt = iter->second;
					events.erase(iter); // remove now

					releaseLock(mapAccess);
					registerEvent(tmpEvt); // reschedule for its next occurance
					acquireLock(mapAccess);

					scanAgain = true;
					break;
				}
				else {
					scanAgain = false; // don't scan anymore since they're in order
					break;
				}
			}
		}
		scanAgain = true;
		releaseLock(mapAccess);
	}
}

void Juggler::Scheduler::schedClock() {
	time_t now;
	struct tm* d;

	while(running) {
		time(&now);
		d = localtime(&now);
		
		// check if there is an event that needs to happen
		acquireLock(mapAccess);
		auto evt = events.begin(); // event with smallest timestamp

		if(evt != events.end() && evt->first <= now) { // this event is overdue
			signalCondition(eventToSchedule);
		}
		
		releaseLock(mapAccess);
		d = localtime(&now); // in case schedWaitLoop took a really long time
		sleep(60 - d->tm_sec); // wake up at every minute
	}
}

string Juggler::Scheduler::timeToString(time_t stamp) {
	struct tm* t = localtime(&stamp);
	char buffer[256];
	strftime(buffer, 256, "%a, %e %b %Y %T LOCAL", t);
	return string(buffer);
}

bool Juggler::Scheduler::registerEvent(std::string des, std::string expression, void (*caller) (void)) {
	smatch matcher;
	regex shape("^([U|M|T|W|R|F|S]*) ([0-9]+):([0-9]+)\\s*\\+*([0-9\\-]+)?\\s*(DST)?$");
	if(regex_search(expression, matcher, shape)) {
		TimeEvent* event = new TimeEvent;
		event->expression = expression;
		event->caller = caller;
		event->des = des;
		return registerEvent(event);
	}
	else {
		return false;
	}
}

bool Juggler::Scheduler::registerEvent(std::string expression, void (*caller) (void)) {
	smatch matcher;
	regex shape("^([U|M|T|W|R|F|S]*) ([0-9]+):([0-9]+)\\s*\\+*([0-9\\-]+)?\\s*(DST)?$");
	if(regex_search(expression, matcher, shape)) {
		TimeEvent* event = new TimeEvent;
		event->expression = expression;
		event->caller = caller;
		event->des = "";
		return registerEvent(event);
	}
	else {
		return false;
	}
}

bool Juggler::Scheduler::dayLightSavingsInEffectNow(time_t t) {
	struct tm* d = localtime(&t); // can only check if the local time is in DST or not
	if(d->tm_isdst > 0) {
		return true;
	}
	return false;
}

void Juggler::Scheduler::convertToStandardTime(TimeEvent* event) {
	smatch matcher;
	regex shape("^([U|M|T|W|R|F|S]*) ([0-9]+):([0-9]+)\\s*\\+*([0-9\\-]+)?\\s*(DST)?$");
	if(regex_search(event->expression, matcher, shape)) {
		int hour = std::stoi(matcher[2]);
		int zone = std::stoi(matcher[4]);
		bool dst = matcher[5] == "DST" ? true : false;
		int dstOffset = dayLightSavingsInEffectNow(time(NULL)) ? 1 : 0;
		zone = dst ? zone + dstOffset : zone; // adjust for daylight savings

		if(zone == 0 || (zone == 1 && dst && dstOffset == 1)) {
			return; // we don't need to do anymore it's already normilized
		}

		string days = "";

		int off = zone * -1;
		if((hour + off) % 24 < hour + off) { // we've gone to the next day
			string val = matcher[1];
			const char* daysArr = "UMTWRFS";
			for(auto it = val.begin(); it != val.end(); ++it) {
				int dayOffset = dayCharToInt(*it);
				days += (char) daysArr[(dayOffset + 1) % 7];
			}
		}
		else { // can keep the same days
			days = matcher[1];
		}
		event->expression = days + " " + std::to_string((hour + off) % 24) + ":" + string(matcher[3]) + string(" +0") + (dst ? " DST" : "");
	}
}

bool Juggler::Scheduler::registerEvent(TimeEvent* event) {
	convertToStandardTime(event);
	//std::cout << event->expression << "\n";
	time_t now;
	time(&now);
	struct tm* d = gmtime(&now);
	time_t thisWeek = now - (time_t) (((d->tm_wday + 1) * 86400) - ((23 - d->tm_hour) * 3600) - ((59 - d->tm_min) * 60) - ( 60 - d->tm_sec));

	time_t nextWeek = thisWeek + 604800;

	smatch matcher;
	regex shape("^([U|M|T|W|R|F|S]*) ([0-9]+):([0-9]+)\\s*\\+*([0-9\\-]+)?\\s*(DST)?$");
	if(regex_search(event->expression, matcher, shape)) {
		int hour = std::stoi(matcher[2]);
		int minute = std::stoi(matcher[3]);
		int zone = std::stoi(matcher[4]);
		bool dst = matcher[5] == "DST" ? true : false;
		
		// TODO verify daylight savings

		time_t nextEvent = 0;
		time_t offset = 0;
		string val = matcher[1];
		for(auto it = val.begin(); it != val.end(); ++it) {
			int dayOffset = dayCharToInt(*it);
			if(dayOffset != -1) {
				time_t thisEvent = thisWeek + (time_t) ((dayOffset * 86400) + (hour * 3600) + (minute * 60));

				// event still to come should not be >= because if this event is the next event to run
				// it can run in a loop for the whole minute
				if(thisEvent > now) {
					offset = thisWeek;
				}
				else { 
					offset = nextWeek;
				}

				thisEvent = offset + (time_t) ((dayOffset * 86400) + (hour * 3600) + (minute * 60));
				// verify daylight savings next week too
				if((nextEvent == 0 || thisEvent < nextEvent) && thisEvent >= now) {
					nextEvent = thisEvent;
				}
			}
			else {
				std::cout << "dayCharToInt returned -1 '" << *it << "'\n";
				return false;
			}
		}

		if(nextEvent > 0) {
			acquireLock(mapAccess);
			events.insert(std::pair<time_t, TimeEvent*>(nextEvent, event));
			releaseLock(mapAccess);
		}
		return true;
	}
	else {
		return false;
	}
}

void Juggler::Scheduler::print() {
	time_t now;
	time(&now);
	std::cout << "events: \n";
	acquireLock(mapAccess);
	for(auto iter = events.begin(); iter != events.end(); ++iter) {
		std::cout << "   " << timeToString(iter->first) << "\n";
	}
	releaseLock(mapAccess);
}

Juggler::Scheduler::~Scheduler() {
	delete m;
}