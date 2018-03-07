#include <iostream>
#include <functional>
#include <unistd.h>
#include <juggler>

int main(int argc, char** argv) {
	int threadCount = 5;
	bool wait = false;
	
	std::cout << "how many threads do you want to run? ";
	std::cin >> threadCount;

	std::cout << "do you want to wait for the threads to complete (y/n)? ";
	char answer;
	std::cin >> answer;
	wait = answer == 'y';

	Juggler::ThreadSync jobs(threadCount);
	jobs.run([&](int id) -> void {
		int sleepTime = id + 1;
		std::cout << "thread #" << id << " is sleeping for " << sleepTime << " seconds\n";
		sleep(sleepTime);
		std::cout << "thread #" << id << " has woken up from its nap\n";
	});

	if(wait) {
		std::cout << "main thread is gonna wait\n";
		jobs.waitForEveryone();
	}
	else {
		std::cout << "we're not waiting for those slow pokes ;)\n";
	}

	std::cout << "exiting\n";
	return 0;	
}