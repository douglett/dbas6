// ----------------------------------------
// A mix of dbas-4 and dbas-5 input methods
// ----------------------------------------
#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
// #include <sstream>
using namespace std;


struct InputFile {
	vector<string> lines;
	int lno = 0, pos = 0;

	int load(const string& fname) {
		// reset
		lines = {};
		lno = 0, pos = 0;
		// load
		fstream fs(fname, ios::in);
		if (!fs.is_open())
			return fprintf(stderr, "error loading file %s\n", fname.c_str()), 1;
		string s;
		while (getline(fs, s))
			lines.push_back(s);
		printf("loaded file: %s (%d)\n", fname.c_str(), (int)lines.size());
		return 0;
	}

};