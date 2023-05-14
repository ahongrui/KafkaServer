CROSS=
CXX = $(CROSS)g++
CC  = $(CROSS)gcc
CPPFLAGS = -I./include\
			-I./include/gssapi\
			-I./includekrb5
LDFLAGS = -lpthread -L./libs -lgssapiv2 -lgssapi_krb5 -lkdb5 -Wl,-rpath,./libs
CFLAGS  = 
CXXFLAGS = -Wall -Wno-strict-aliasing -Wno-unused-variable -Wno-literal-suffix -std=c++11
OPT = -g -O1
TARGET = kafkaserver
SRCS = $(wildcard src/*.c) $(wildcard src/*.cpp)
OBJS = $(addsuffix .o, $(basename $(SRCS)))

all: $(TARGET)
$(TARGET): $(OBJS)
	$(CXX) $(OPTS) $(LDFLAGS) -o $@ $^

.cpp.o:
	$(CXX) -c $(OPTS) $(CPPFLAGS) $(CXXFLAGS) $< -o $@
.c.o:
	$(CC) -c $(OPTS) $(CPPFLAGS) $(CFLAGS) $< -o $@
clean:
	rm $(OBJS) $(TARGET)
	rm -f *~
