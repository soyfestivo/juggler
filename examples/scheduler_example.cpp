#include <iostream>
#include <functional>
#include <unistd.h>
#include <juggler>

void testFunc() {
	std::cout << "testFunc just run!!!\n";
}

int main(int argc, char** argv) {
	Juggler::Scheduler sched;

	sched.registerEvent("this is to test the scheduler", "UMTWRFS 16:57 -6 DST", testFunc);

	// to keep the main thread from exiting, killing the scheduler
	int pause;
	std::cin >> pause;
	return 0;	
}