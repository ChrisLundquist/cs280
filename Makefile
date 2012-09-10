CFLAGS = -g

test: test1

test1: all
	./test1.exe

test2:
	./test2.exe

all: driver1 driver2

driver1:
	g++ -o test1.exe $(CFLAGS) driver-sample.cpp ObjectAllocator.cpp PRNG.cpp -Werror -Wall -Wextra -Wconversion -ansi -pedantic

driver2:
	g++ -o test2.exe $(CFLAGS) driver-sample2.cpp ObjectAllocator.cpp PRNG.cpp -Werror -Wall -Wextra -Wconversion -ansi -pedantic

clean:
	rm *.exe
