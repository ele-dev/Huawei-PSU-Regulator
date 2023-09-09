# Linux Makefile to compile c++ application on/for UNIX based systems (x86 or arm)
# written by Elias Geiger

CXXFLAGS = -std=c++17

app: main.o UdpReceiver.o Queue.o PsuController.o
	g++ -o ../bin/regulatorApp main.o UdpReceiver.o Queue.o PsuController.o -pthread

main.o:
	g++ -c main.cpp

UdpReceiver.o:
	g++ -c UdpReceiver.cpp

Queue.o:
	g++ -c Queue.cpp

PsuController.o:
	g++ -c PsuController.cpp

clean: 	
	rm *.o ../bin/regulatorApp
