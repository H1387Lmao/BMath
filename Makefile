CXX = clang++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
LDFLAGS ?=

SRCS := src/parser.cpp src/main.cpp
OBJS := $(SRCS:src/%.cpp=bin/%.o)
DEPS := src/lexer.hpp src/parser.hpp src/node.hpp src/token.hpp src/codegen.hpp

.PHONY: linux windows clean

linux: $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o bin/bmath

windows: $(OBJS)
	g++ $(LDFLAGS) $(OBJS) -o bin/bmath.exe

bin/%.o: src/%.cpp $(DEPS)
	@mkdir -p bin
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf bin/*

