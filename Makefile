all: dd3o

dd3o: main.cxx
	g++ -o dd3o.exe main.cxx -std=c++11 -Wall -lpthread -lncurses

clean:
	rm dd3o.exe

clean-logs:
	rm logs/*.txt
