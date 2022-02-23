CCFLAGS= -std=gnu++11 -pedantic -Wall -Werror -ggdb3 -pthread

proxyd: proxyd.o proxy.o networks.o httpParser.o
	g++ -o proxyd $(CCFLAGS) proxyd.o proxy.o networks.o httpParser.o

%.o: %.cpp proxy.hpp networks.hpp httpParser.hpp response.hpp request.hpp cache.hpp
	g++ -c $(CCFLAGS) $<

clean:
	rm -f *.o  *~ proxyd  
