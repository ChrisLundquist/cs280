test1: all
	./test1.exe
	./test2.exe

all: driver1 driver2

driver1:
	g++ -o test1.exe -g driver-sample.cpp ObjectAllocator.cpp PRNG.cpp -Werror -Wall -Wextra -Wconversion -ansi -pedantic

driver2:
	g++ -o test2.exe -g driver-sample2.cpp ObjectAllocator.cpp PRNG.cpp -Werror -Wall -Wextra -Wconversion -ansi -pedantic

clean:
	rm *.exe
