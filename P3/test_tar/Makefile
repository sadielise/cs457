CXXFLAGS=-Wall -g -I.
all: router manager
router:router.o
	g++ $(CXXFLAGS) router.o -o router -lpthread
manager:manager.o
	g++ $(CXXFLAGS) manager.o -o manager
router.o:router.cpp
	g++ $(CXXFLAGS) -c router.cpp
manager.o:manager.cpp
	g++ $(CXXFLAGS) -c manager.cpp
clean:
	rm -f *.o router manager
