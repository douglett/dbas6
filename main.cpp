#include <iostream>
#include "inputfile.hpp"
using namespace std;


int main() {
	printf("hello world\n");

	InputFile inp;
	inp.load("scripts/test.bas");
}