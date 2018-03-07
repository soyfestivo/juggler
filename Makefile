FLAGS = -std=c++11 -O2
LIBS = -lpthread

thread_manager.o:
	g++ -fpic $(FLAGS) -c lib/thread_manager.cpp -o thread_manager.o

thread_sync.o:
	g++ -fpic $(FLAGS) -c lib/thread_sync.cpp -o thread_sync.o

all: thread_manager.o thread_sync.o
	g++ -shared $(FLAGS) -o libJuggler.so thread_manager.o thread_sync.o

examples: all examples/example_1.cpp
	g++ -I. -L. $(FLAGS) $(LIBS) -lJuggler examples/example_1.cpp -o example_1

clean:
	-rm *.o
	-rm *.so
	-rm example_1