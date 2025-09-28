CXX = clang++
CXXFLAGS ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic
LDFLAGS ?=

OBJS := parser.o main.o
DEPS := lexer.hpp parser.hpp node.hpp token.hpp codegen.hpp

.PHONY: all clean

all: bmath

bmath: $(OBJS)
	$(CXX) $(LDFLAGS) $(OBJS) -o $@

%.o: %.cpp $(DEPS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) bmath bmath.exe
