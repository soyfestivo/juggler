#ifndef _THREADMANAGER_H_
#define _THREADMANAGER_H_

#include <iostream>
#include <time.h>
#include <pthread.h>
#include <iostream>
#include <sstream>
#include <vector>

#define Lock pthread_mutex_t
#define Condition pthread_cond_t

#define initLock(x) pthread_mutex_init(&(x), 0)
#define initCondition(x) pthread_cond_init(&(x), 0)

#define acquireLock(x) pthread_mutex_lock(&(x))
#define releaseLock(x) pthread_mutex_unlock(&(x))

#define destroyLock(x) pthread_mutex_destroy(&(x))
#define destroyCondition(x) pthread_cond_destroy(&(x))

// should be holding the mutex before 
#define waitOnCondition(x, mutex) pthread_cond_wait(&(x), &(mutex))

// should be holding the mutex from waitOnCondition
#define signalCondition(x) pthread_cond_broadcast(&(x))


namespace Juggler {
	class ThreadManager {
	private:
		static std::vector<void (*) (pthread_t)> threadStartHooks;
		static std::vector<void (*) (pthread_t)> threadReleaseHooks;

		static void runThreadStartHooks(pthread_t tid);
		static void runThreadReleaseHooks(pthread_t tid);

		friend void* launch(void* index);

		int tMax;
		int threadCount;
		struct thread* table;

		void releaseFromGlobalTree(struct thread* thread);
		void addToGlobalTree(struct thread* thread, std::string label);

	public:
		Lock lock;
		ThreadManager();
		ThreadManager(int max);

		static void printThreads(std::iostream& stream);
		static void addThreadStartHook(void (*caller) (pthread_t));
		static void addThreadReleaseHook(void (*caller) (pthread_t));

		int reap();
		int lease(void (*func) (void*), void* var, std::string description = "default");
		bool kill(int id);
		void subtract();
		int active();
		void waitOn(int id);
		~ThreadManager();
	};

	enum status { FREE, USED, DEAD };

	struct thread {
		enum status status;
		pthread_t tid;
		time_t startTime;
		void (*func) (void*);
		void* var;
		ThreadManager* manager;
	};
}

#endif