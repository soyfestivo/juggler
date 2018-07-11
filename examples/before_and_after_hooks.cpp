#include <iostream>
#include <functional>
#include <unistd.h>
#include <juggler>

void printThreadID(pthread_t tid) {
	std::cout << "Hello my tid is: " << tid << "\n";
}

void printExitingThread(pthread_t tid) { 
	std::cout << "Now exiting tid " << tid << "\n";
}

int main(int argc, char** argv) {
  Juggler::ThreadManager::addThreadStartHook(&printThreadID);
  Juggler::ThreadManager::addThreadReleaseHook(&printExitingThread);

	Juggler::ThreadSync jobs(2);

	std::cout << "Hello from main thread" << "\n";
	jobs.run([&](int id) -> void {
    sleep(id + 1);
		std::cout << "hello from worker #" << id << "\n";
		sleep(5);
	});

  std::cout << "gonna wait\n";
	jobs.waitForEveryone();

	std::cout << "exiting main thread\n";
	return 0;	
}