CXX = g++
CC = gcc
CXXFLAGS = -std=c++17 -O2 -Wall -Iinclude -D_WIN32_WINNT=0x0601
CFLAGS = -O2 -Iinclude
LDFLAGS = -lws2_32 -lmswsock -lpthread

TARGET = DisasterReliefSystem.exe

SRCS = src/main.cpp \
       src/database.cpp \
       src/disaster.cpp \
       src/resource.cpp \
       src/relief_request.cpp \
       src/distribution.cpp \
       src/system.cpp

all: $(TARGET)

build/sqlite3.o: src/sqlite3.c
	$(CC) $(CFLAGS) -c src/sqlite3.c -o build/sqlite3.o

$(TARGET): build/sqlite3.o $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) build/sqlite3.o $(LDFLAGS) -o $(TARGET)
	@echo Build completed successfully: $(TARGET)

clean:
	rm -f $(TARGET) build/sqlite3.o

run: $(TARGET)
	./$(TARGET) 8080
