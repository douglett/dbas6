#include <iostream>
#include <fstream>
#include "inputfile.hpp"
#include "parser.hpp"
#include "asm.hpp"
using namespace std;


int main() {
	printf("dbas-6 running...\n");
	// printf("---\n");

	Parser p;
	p.load("scripts/test.bas");

	p.p_program();
	// printf("---\n");
	fstream fs("bin/output.asm", ios::out);
	for (const auto& ln : p.prog2) {
		// cout << ln << endl;
		fs   << ln << endl;
	}
	fs.close();
	printf("---\n");

	ASM a;
	a.load("bin/output.asm");
	a.mainloop();
}