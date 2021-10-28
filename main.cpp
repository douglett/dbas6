#include <iostream>
#include "inputfile.hpp"
#include "parser.hpp"
using namespace std;


int main() {
	printf("hello world\n");

	InputFile inp;
	inp.load("scripts/test.bas");

	int r  = inp.peek("'print literal endl");
	int r2 = inp.peek("'print literal");
	int r3 = inp.peek("'print endl");
	printf("results:  %d  %d  %d \n", r, r2, r3);
	printf("---\n");


	Parser p;
	p.load("scripts/test.bas");
	p.nextline();
	p.p_type();
}