CC = g++
TARGET = $(wildcard *.cpp)
CPPFLAGS = -O2 -std=c++11 -Wall -Wextra -pedantic 
LDFLAGS = 

nessdl: $(TARGET)
	$(CC) $(LDFLAGS) $(CPPFLAGS) $^ -o $@ 

compile:
	$(CC) $(LDFLAGS) $(CPPFLAGS) $(source) -o $(output)
