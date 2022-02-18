CCFLAGS= -std=gnu++11 -pedantic -Wall -ggdb3 -pthread
TARGET = proxy testParser 

TARGET: $(TARGET)

testParser: testParser.o httpParser.o
	g++ -o testParser $(CCFLAGS) testParser.o httpParser.o

proxy: proxy.o networks.o httpParser.o
	g++ -o proxy $(CCFLAGS) proxy.o networks.o  httpParser.o

%.o: %.cpp networks.hpp httpParser.hpp response.hpp request.hpp cache.hpp
	g++ -c $(CCFLAGS) $<

clean:
	rm -f *.o  *~ proxy testParser 
