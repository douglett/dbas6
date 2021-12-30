// ----------------------------------------
// Emit ASM code
// ----------------------------------------
#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <ctime>
using namespace std;


struct Emitter {
	const string COMMENT_SEP = "\t ; ";
	vector<string> output, subsection;
	// int flag_lastret = 0;  // emit flags
	

	void emit(const vector<string>& vs, const string& c="") {
		// flag_lastret = (vs.size() && vs.at(0) == "ret");
		output.push_back( "\t" + join(vs) + (c.length() ? COMMENT_SEP + c : "") );
	}
	void emitsub(const vector<string>& vs, const string& c="") {
		// emit-subsection (is there a better name for this?)
		// flag_lastret = (vs.size() && vs.at(0) == "ret") * 2;
		subsection.push_back( "\t" + join(vs) + (c.length() ? COMMENT_SEP + c : "") );
	}
	void joinsub() {
		output.insert( output.end(), subsection.begin(), subsection.end() );
		subsection = {};
	}
	void label(const string& s) {
		output.push_back( s + ":" );
	}
	void comment(const string& c) {
		output.push_back( "\t; " + c );
	}
	void topcomment(const string& c) {
		output.push_back( ";;; " + c );
	}
	void header() {
		topcomment("");
		time_t now = time(NULL);
		string timestr = ctime(&now);
		timestr.pop_back();
		topcomment("compiled on:  " + timestr);
		topcomment("");
	}

	// void           lstack_push(const string& label) { while_labels.push_back(label); }
	// void           lstack_pop() { while_labels.pop_back(); }
	// void           lstack_emit(const string& suffix) { label( lstack_top() + suffix ); }
	// void           lstack_emit(int suffix) { label( lstack_top() + to_string(suffix) ); }
	// const string&  lstack_top() { return while_labels.at( while_labels.size()-1 ); }
	// const string&  lstack_at(int pos) { return while_labels.at( while_labels.size()-1 ); }


	int outputfile(const string& fname) {
		fstream fs(fname, ios::out);
		if (!fs.is_open())
			return fprintf(stderr, "error opening file: %s\n", fname.c_str()), 1;
		for (const auto& ln : output)
			fs << ln << endl;
		return 0;
	}
};