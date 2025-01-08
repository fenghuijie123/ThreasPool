main: main.cpp
	g++ -o main -std=c++11 -Wall -g main.cpp -lpthread
clean:
	rm main
