CXX = g++
CXXFLAGS = -std=c++17 -Iinclude -Wall -Wextra


LIBDIR = lib
LIB = $(LIBDIR)/libcore.a


MAIN = src/main.cpp
TARGET = test

all: $(TARGET)


$(TARGET): $(MAIN) $(LIB)
	$(CXX) $(CXXFLAGS) $(MAIN) -L$(LIBDIR) -lcore -lSDL2 -lGL -lm  -o $@

clean:
	rm -f $(TARGET)

.PHONY: all clean

