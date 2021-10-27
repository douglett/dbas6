#include <iostream>
#include "inputfile.hpp"
using namespace std;


int main() {
	printf("hello world\n");

	InputFile inp;
	inp.load("scripts/test.bas");
	inp.tokenizeline();

	int r  = inp.peek("'print literal endl");
	int r2 = inp.peek("'print literal");
	int r3 = inp.peek("'print endl");
	printf("results:  %d  %d  %d \n", r, r2, r3);
}