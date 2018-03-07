#include "ThreadSync.h"

namespace Juggler {
	ThreadSync::ThreadSync(int max) {
		initLock(lock);
		initLock(condLock);
		initCondition(everyoneIsDone);
		countUntil = max;
		count = 0;
		manager = new ThreadManager(countUntil);
	}

	void ThreadSync::waitForEveryone() {
		acquireLock(lock);
		if(count >= countUntil) {
			releaseLock(lock);
			return;
		}
		releaseLock(lock);

		acquireLock(condLock);
		waitOnCondition(everyoneIsDone, condLock);
		releaseLock(condLock);
	}

	void ThreadSync::done() {
		acquireLock(lock);
		count++;
		if(count >= countUntil) {
			acquireLock(condLock); // must be holding it before we call signal
			signalCondition(everyoneIsDone);
			releaseLock(condLock);
		}
		releaseLock(lock);
	}

	typedef struct thread_sync_pass {
		ThreadSync* ts;
		int id;
		std::function<void (int id)> lambda;
	} ThreadSyncPass;

	void threadSyncThreadRun(void* data) {
		ThreadSyncPass* t = (ThreadSyncPass*) data;
		t->lambda(t->id);
		t->ts->done();
		delete t;
	}

	void ThreadSync::run(std::function<void (int id)> lambda, std::string des) {
		if(countUntil == 1) { // we're only running it once, so just do it outright
			lambda(0);
			return;
		}

		for(int i = 0; i < countUntil; i++) {
			ThreadSyncPass* tsp = new ThreadSyncPass;
			tsp->ts = this;
			tsp->id = i;
			tsp->lambda = lambda;
			int id = manager->lease(threadSyncThreadRun, (void*) tsp, des);
			if(id == -1) {
				std::cout << "ThreadSync::run ERROR could not spawn new thread???\n";
			}
		}
	}

	ThreadSync::~ThreadSync() {
		acquireLock(lock);
		if(count >= countUntil) {
			signalCondition(everyoneIsDone);
		}
		else {
			std::cout << "ERROR ThreadSync::~ThreadSync destructor called before every thread was done. Tell them we are all done now\n";
			signalCondition(everyoneIsDone);
		}
		releaseLock(lock);

		destroyLock(lock);
		destroyLock(condLock);
		destroyCondition(everyoneIsDone);
		delete manager;
	}
}