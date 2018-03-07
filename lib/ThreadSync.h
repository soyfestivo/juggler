#ifndef _THREAD_SYNC_H_
#define _THREAD_SYNC_H_

#include <iostream>
#include <functional>

#include "ThreadManager.h"

namespace Juggler {
	class ThreadSync {
	private:
		Lock lock;
		Lock condLock;
		Condition everyoneIsDone;
		ThreadManager* manager;
		int count;
		int countUntil;

	public:
		ThreadSync(int max);

		void run(std::function<void (int id)> lambda, std::string des = "threadsync job");
		void waitForEveryone();
		void done();

		~ThreadSync();
	};
}

#endif