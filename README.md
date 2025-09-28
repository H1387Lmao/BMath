# BMath
## Basic Math.

### Testing my c++ skills on a simple programming language.

## How to compile

## Linux

```sh
make all
```

## Windows (With MYSYS G++)

```bat
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -c parser.cpp -o parser.o
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -c main.cpp -o main.o
g++  parser.o main.o -o bmath
```
