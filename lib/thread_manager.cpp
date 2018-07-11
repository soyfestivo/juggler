#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <map>

#include "thread_manager.h"

std::map<pthread_t, std::string> threadManagerMap;
Lock threadManagerMapLock;
bool threadManagerMapLockSetup = false;

std::vector<void (*) (pthread_t)> Juggler::ThreadManager::threadStartHooks;
std::vector<void (*) (pthread_t)> Juggler::ThreadManager::threadReleaseHooks;

namespace Juggler {
	void* launch(void* index) {
		// cannot be cancled? so we can use condition variables
		pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
		struct thread* thread = (struct thread*) index;
		
		// run thread start hooks if any
		ThreadManager::runThreadStartHooks(thread->tid);		

		// start the actual call
		thread->func(thread->var);

 		// run thread release hooks if any
		ThreadManager::runThreadReleaseHooks(thread->tid);

		// clear it from the global tree
		thread->manager->releaseFromGlobalTree(thread);

		acquireLock(thread->manager->lock);
		thread->status = DEAD;
		thread->manager->subtract();
		releaseLock(thread->manager->lock);

		pthread_exit(NULL);
	}

	void ThreadManager::addThreadStartHook(void (*caller) (pthread_t)) { 
		ThreadManager::threadStartHooks.push_back(caller);
	}
	
	void ThreadManager::addThreadReleaseHook(void (*caller) (pthread_t)) {
		ThreadManager::threadReleaseHooks.push_back(caller);
	}

	void ThreadManager::runThreadStartHooks(pthread_t tid) {
		acquireLock(threadManagerMapLock);
		if(ThreadManager::threadStartHooks.size() > 0) {
			for(auto hook = ThreadManager::threadStartHooks.begin(); hook != ThreadManager::threadStartHooks.end(); ++hook) {
				(*hook)(tid);
			}
		}
		releaseLock(threadManagerMapLock);
	}

	void ThreadManager::runThreadReleaseHooks(pthread_t tid) {
		acquireLock(threadManagerMapLock);
		if(ThreadManager::threadReleaseHooks.size() > 0) {
			for(auto hook = ThreadManager::threadReleaseHooks.begin(); hook != ThreadManager::threadReleaseHooks.end(); ++hook) {
				(*hook)(tid);
			}
		}
		releaseLock(threadManagerMapLock);
	}

	void ThreadManager::printThreads(std::iostream& stream) {
		acquireLock(threadManagerMapLock);
		for(auto iter = threadManagerMap.begin(); iter != threadManagerMap.end(); ++iter) {
			stream << iter->first << ": " << iter->second << "\n";
		}
		releaseLock(threadManagerMapLock);
	}

	void ThreadManager::releaseFromGlobalTree(struct thread* thread) {
		acquireLock(threadManagerMapLock);
		auto thisThread = threadManagerMap.find(thread->tid);
		if(thisThread != threadManagerMap.end()) {
			threadManagerMap.erase(thisThread);
		}
		releaseLock(threadManagerMapLock);
	}

	void ThreadManager::addToGlobalTree(struct thread* thread, std::string label) {
		acquireLock(threadManagerMapLock);
		threadManagerMap.insert(std::pair<pthread_t, std::string>(thread->tid, label));
		releaseLock(threadManagerMapLock);
	}

	bool ThreadManager::kill(int id) {
		acquireLock(lock);
		pthread_cancel(table[id].tid);
		releaseLock(lock);

		releaseFromGlobalTree(&table[id]);
		return true;
	}

	ThreadManager::ThreadManager(int max) {
		if(threadManagerMapLockSetup == false) {
			threadManagerMapLockSetup = true;
			initLock(threadManagerMapLock);
		}
		table = (struct thread*) malloc(sizeof(struct thread) * max);
		tMax = max;
		threadCount = 0;

		initLock(lock);

		for(int i = 0; i < max; i++) {
			table[i].status = FREE;
			table[i].manager = this;
		}
	}

	int ThreadManager::reap() {
		struct thread* t;
		int count = 0;
		acquireLock(lock);
		for(int i = 0; i < tMax; i++) {
			t = &table[i];
			if(t->status == DEAD) {
				pthread_join(t->tid, NULL);
				t->status = FREE;
				count++;
			}
		}
		releaseLock(lock);
		return count;
	}

	void ThreadManager::waitOn(int id) {
		if(id < tMax && id >= 0 && table[id].status == USED) {
			pthread_join(table[id].tid, NULL);

			acquireLock(lock);
			table[id].status = FREE;
			releaseLock(lock);
		}
	}

	int ThreadManager::lease(void (*func) (void*), void* var, std::string description) {
		struct thread* t;
		acquireLock(lock);
		for(int i = 0; i < tMax; i++) {
			t = &table[i];
			if(t->status == FREE || t->status == DEAD) {
				if(t->status == DEAD) {
					pthread_join(t->tid, NULL);
				}
				t->status = USED;
				t->func = func;
				t->var = var;
				t->startTime = time(NULL);
				if(pthread_create(&(table[i].tid), NULL, launch, (void*) &table[i]) == 0) {
					threadCount++;
					releaseLock(lock);
					addToGlobalTree(t, description);
					return i;
				}
				else {
					t->status = FREE;
					releaseLock(lock);
					return -1;
				}
			}
		}
		releaseLock(lock);
		return -1;
	}

	int ThreadManager::active() {
		return threadCount;
	}

	void ThreadManager::subtract() {
		threadCount--;
	}

	ThreadManager::~ThreadManager() {
		destroyLock(lock);
		free(table); // shouldn't we reap the threads?
	}
}