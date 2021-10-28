// ----------------------------------------
// Basic parser main
// ----------------------------------------
#pragma once
#include "helpers.hpp"
#include "inputfile.hpp"
#include <map>
using namespace std;


struct Prog {
	struct Dim  { string name, type; };
	struct Type { string name; map<string, Dim> members; };
	struct Func { string name; map<string, Dim> args; };
};

const vector<string> BASIC_KEYWORDS = {
	"int", "string",
	"type", "dim", "function", "end"
	"if", "while", "break", "continue",
};


struct Parser : InputFile {
	Node prog;
	map<string, Prog::Type> types;
	map<string, Prog::Dim>  globals, locals;
	map<string, Prog::Func> functions;

	void p_program() {
		prog = Node::List();
		prog.pushtoken("program");
		p_section("type", prog);
		p_section("dimglobal", prog);
		p_section("function", prog);
	}

	void p_section(const string& section, Node& p) {
		Node& nn = p.pushlist();
		nn.pushtokens({ "section", section });
		while (!eof())
			if      (expect("endl"))  nextline();
			else if (section == "type"      && peek("'type"))      p_type(nn);
			else if (section == "dimglobal" && peek("'dim"))       p_dimglobal(nn);
			else if (section == "function"  && peek("'function"))  p_function(nn);
			else    break;
	}

	void p_type(Node& p) {
		require("'type @identifier endl");
		string tname = presults.at(0);
			namecollision(tname);
			types[tname] = { tname };
		Node& nn = p.pushlist();
			nn.pushtokens({ "type", tname });
		nextline();
		// type members
		while (!eof()) {
			string name, type;
			if      (expect("endl"))  { nextline();  continue; }
			else if (expect("'dim @identifier @identifier endl"))  name = presults.at(1), type = presults.at(0);
			else if (expect("'dim @identifier endl"))  name = presults.at(0), type = "int";
			else    break;
			if (type == "integer")  type = "int";
				typecheck(type);
				namecollision(name);
				if (types[tname].members.count(name))  error();  // additional name collision inside this type
				types[tname].members[name] = { name, type };  // save type member
			nn.pushlist().pushtokens({ "dim", name, type });
			nextline();
		}
		// end type
		require("'end 'type endl");
		nextline();
	}
	
	void p_dimglobal(Node& p) {
		string name, type;
		if      (expect("'dim @identifier @identifier"))  name = presults.at(1), type = presults.at(0);
		else if (require("'dim @identifier endl"))  name = presults.at(0), type = "int";
		if (type == "integer")  type = "int";
			typecheck(type);
			namecollision(name);
			globals[name] = { name, type };
		Node& nn = p.pushlist();
		nn.pushtokens({ "dim", name, type });
		nextline();
	}

	void p_function(Node& p) {
		require("'function @identifier '(");
		string fname = presults.at(0);
			namecollision(fname);
			functions[fname] = { fname };
		auto& nn = p.pushlist();
			nn.pushtokens({ "function", fname });

		// arguments
		auto& args = nn.pushlist();
			args.pushtoken("args");
		// while (!eol()) {
		// 	if (peek("')"))  break;
		// }
		require("') endl");
		nextline();

		// block

		// end function
		require("'end 'function endl");
		nextline();
	}



	// helpers
	int error() {
		throw DBError(lno);
	}
	int typecheck(const string& type) {
		if (type != "int" && type != "string" && types.count(type) == 0)  error();
		return 0;
	}
	int namecollision(const string& name) {
		// TODO: be careful with this function, its a bit WIP
		for (auto& k : BASIC_KEYWORDS)
			if (name == k)  error();
		if (types.count(name))      error();
		if (functions.count(name))  error();
		if (locals.count(name))     error();
		// note: allow name shadowing here
		if (globals.count(name))    error();
		return 0;
	}
};