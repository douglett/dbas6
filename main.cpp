#include <iostream>
#include "inputfile.hpp"
#include "parser.hpp"
#include "asm.hpp"
using namespace std;


int main() {
	printf("dbas-6 running...\n");
	// printf("---\n");

	Parser p;
	p.load("scripts/string.bas");
	p.p_program();
	p.em.outputfile("bin/output.asm");
	printf("---\n");

	ASM a;
	a.load("bin/output.asm");
	a.mainloop();
	printf("---\n");
	a.showstate();
}