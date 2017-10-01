CXXFLAGS=-Wall -g -I.
all: awget ss
awget:awget.o
	g++ $(CXXFLAGS) awget.o -o awget
ss:ss.o
	g++ $(CXXFLAGS) ss.o -o ss -lpthread
awget.o:awget.cpp
	g++ $(CXXFLAGS) -c awget.cpp
ss.o:ss.cpp
	g++ $(CXXFLAGS) -c ss.cpp
clean:
	rm -f *.o awget ss
