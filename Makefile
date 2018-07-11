FLAGS = -std=c++11 -O2
LIBS = -lpthread

thread_manager.o:
	g++ -fpic $(FLAGS) -c lib/thread_manager.cpp -o thread_manager.o

thread_sync.o:
	g++ -fpic $(FLAGS) -c lib/thread_sync.cpp -o thread_sync.o

scheduler.o:
	g++ -fpic $(FLAGS) -c lib/scheduler.cpp -o scheduler.o

all: thread_manager.o thread_sync.o scheduler.o
	g++ -shared $(FLAGS) -o libJuggler.so thread_manager.o thread_sync.o scheduler.o

install: all
	cp juggler.h /usr/local/include/juggler
	-mkdir /usr/local/include/juggler_lib
	cp -r lib/*.h /usr/local/include/juggler_lib
	cp libJuggler.so /usr/local/lib

uninstall: clean
	-rm -r /usr/local/include/juggler_lib
	-rm /usr/local/include/juggler
	-rm /usr/local/lib/libJuggler.so

examples: all
	g++ -I. -L. $(FLAGS) $(LIBS) -lJuggler examples/example_1.cpp -o example_1
	g++ -I. -L. $(FLAGS) $(LIBS) -lJuggler examples/scheduler_example.cpp -o scheduler_example
	g++ -I. -L. $(FLAGS) $(LIBS) -lJuggler examples/before_and_after_hooks.cpp -o before_and_after_hooks

clean:
	-rm *.o
	-rm *.so
	-rm example_1
	-rm scheduler_example
	-rm before_and_after_hooks
