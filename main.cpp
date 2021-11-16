#include <iostream>
#include "inputfile.hpp"
#include "parser.hpp"
#include "runtime.hpp"
#include <iostream>
using namespace std;


void inputtest() {
	InputFile inp;
	inp.loadstring("print \"hello world\"");

	int r  = inp.peek("'print literal endl");
	int r2 = inp.peek("'print literal");
	int r3 = inp.peek("'print endl");
	printf("results:  %d  %d  %d \n", r, r2, r3);
	printf("---\n");
}


int main() {
	printf("hello world\n");
	printf("---\n");
	// inputtest();

	Parser p;
	// p.load("scripts/test2.bas");
	p.load("scripts/advent.bas");
	p.p_program();
	p.prog.show();
	printf("---\n");

	Runtime r(p.prog);
	// r.flag_memtrace = 1;
	r.r_prog();
}