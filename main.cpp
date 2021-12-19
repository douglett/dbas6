#include <iostream>
#include "inputfile.hpp"
#include "parser.hpp"
using namespace std;


int main() {
	printf("hello world\n");
	printf("---\n");

	Parser p;
	p.load("scripts/test.bas");

	p.p_program();
	p.prog.show();
	printf("---\n");
	for (const auto& ln : p.prog2)  cout << ln << endl;
	printf("---\n");
}