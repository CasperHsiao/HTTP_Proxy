CCFLAGS= -std=gnu++98 -pedantic -Wall -ggdb3

proxy: proxy.o networks.o
	g++ -o proxy $(CCFLAGS) proxy.o networks.o

%.o: %.cpp networks.hpp
	g++ -c $(CCFLAGS) $<

clean:
	rm -f *.o  *~ proxy
