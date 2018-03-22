# Juggler
*Simple thread and batch management in C++*

Running batch jobs in multiple threads in C++ made easy with `ThreadSync` and running arbitrary threads is made easy with `ThreadManager`

## Installation
Installation is easy! All you need to do is compile:
```bash
$ make install
```
Note that install is putting the juggler `.so` in `/usr/local/lib` and the headers in `/usr/local/includes`. Be sure that these are included in your compiler's path for correct linkage.

And then add when compiling make sure to include the location of the lib with both `-l` like so:
```bash
$ g++ -lJuggler -lpthread my_files.cpp ... -o ...
```
## Example
A very simple example of batch job managment:
```C++
Juggler::ThreadSync batches(JOB_COUNT);

batches.run([](int id) {
	sleep(id); // sleep for some number of seconds
	std::cout << "thread #" << id << "ending\n";
});

batches.waitForEveryone(); // this way the main thread does not terminate the program before the child threads are done running.
```
For signaling jobs that they should complete early you can simply put a `volatile bool` as the contition to keep running the specific jobs.