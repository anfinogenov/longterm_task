all: dd3o

dd3o: main.cxx
	g++ -o dd3o.exe main.cxx -std=c++11 -Wall -L. -lbass -Wl,-rpath,. -lpthread -lncurses

clean:
	rm dd3o.exe

clean-logs:
	rm logs/log*.txt
