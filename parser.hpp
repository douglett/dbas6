// ----------------------------------------
// Basic parser main
// ----------------------------------------
#pragma once
#include "inputfile.hpp"
using namespace std;


struct Parser : InputFile {
	void p_type() {
		require("'type @identifier endl");
		string tname = presults.at(0);
		printf("type: %s\n", tname.c_str());
		nextline();
		while (!eof()) {
			if      (expect("endl")) ;
			else if (expect("'dim @identifier @identifier endl"))
				printf("type-member: %s, %s %s\n", tname.c_str(), presults.at(0).c_str(), presults.at(1).c_str());
			else if (expect("'dim @identifier endl"))
				printf("type-member: %s, int %s\n", tname.c_str(), presults.at(0).c_str());
			else    break;
			nextline();
		}
		require("'end 'type endl");
	}
};