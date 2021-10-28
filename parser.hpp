// ----------------------------------------
// Basic parser main
// ----------------------------------------
#pragma once
#include "helpers.hpp"
#include "inputfile.hpp"
using namespace std;


struct Parser : InputFile {
	Node prog;

	void p_program() {
		prog = Node::List();
		prog.pushtoken("program");
		p_section("type", prog);
	}

	void p_section(const string& section, Node& p) {
		Node& nn = p.pushlist();
		nn.pushtoken("section");
		while (!eof())
			if      (expect("endl"))  nextline();
			else if (section == "type"      && peek("'type"))      p_type(nn);
			// else if (section == "dimglobal" && peek("'dim"))       p_dimglobal();
			// else if (section == "function"  && peek("'function"))  p_function();
			else    break;
	}

	void p_type(Node& p) {
		require("'type @identifier endl");
		string tname = presults.at(0);
		Node& nn = p.pushlist();
		nn.pushtokens({ "type", tname });
		nextline();
		while (!eof()) {
			if      (expect("endl")) ;
			else if (expect("'dim @identifier @identifier endl"))
				nn.pushlist().pushtokens({ "dim", presults.at(0), presults.at(1) });
			else if (expect("'dim @identifier endl"))
				nn.pushlist().pushtokens({ "dim", "int", presults.at(0) });
			else    break;
			nextline();
		}
		require("'end 'type endl");
		// printf("---\n");
		// nn.show();
		// printf("---\n");
	}
};