#include <iostream>
#include <fstream>
#include "inputfile.hpp"
#include "parser.hpp"
#include "asm.hpp"
using namespace std;


int main() {
	printf("hello world\n");
	printf("---\n");

	Parser p;
	p.load("scripts/test.bas");

	p.p_program();
	// p.prog.show();
	printf("---\n");
	fstream fs("bin/output.asm", ios::out);
	for (const auto& ln : p.prog2) {
		cout << ln << endl;
		fs   << ln << endl;
	}
	printf("---\n");
}