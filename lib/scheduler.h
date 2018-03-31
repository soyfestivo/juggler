#ifndef _SCHEDULER_H_
#define _SCHEDULER_H_
#include <iostream>
#include <map>
#include <regex>
#include <ctime>

#include "thread_manager.h"

typedef struct event {
	std::string expression;
	std::string des;
	void (*caller) (void);
} TimeEvent;

namespace Juggler {
	class Scheduler {
	private:
		Juggler::ThreadManager* m;
		std::map<time_t, TimeEvent*> events;
		Lock mapAccess;
		Condition eventToSchedule;
		bool running;

		// current time, needed for syncing timezones and daylight savings
		int myTimeZone;
		bool daylightSavings;

		void setupDateInfo();

		void schedWaitLoop();
		void schedClock();

		void convertToStandardTime(TimeEvent* event);

		int dayCharToInt(char c);
		bool registerEvent(TimeEvent* event);

		bool dayLightSavingsInEffectNow(time_t t);

	public:
		Scheduler();
		~Scheduler();
		bool registerEvent(std::string expression, void (*caller) (void));
		bool registerEvent(std::string des, std::string expression, void (*caller) (void));
		std::string upNext();
		std::string timeToString(time_t stamp);
		void print();

		friend void schedLaunchA(void* data);
		friend void schedLaunchB(void* data);
	};
}
#endif