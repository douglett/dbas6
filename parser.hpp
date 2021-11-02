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
	struct Func { string name; map<string, Dim> args, locals; };
};

const vector<string> BASIC_KEYWORDS = {
	"int", "string",
	"type", "dim", "function", "end"
	"if", "while", "break", "continue",
};


struct Parser : InputFile {
	Node prog;
	map<string, Prog::Type> types;
	map<string, Prog::Dim>  globals;
	map<string, Prog::Func> functions;
	string cfuncname, ctypename;


	// --- Program structure parsing ---

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
			else if (section == "dimglobal" && peek("'dim"))       p_dim(nn);
			else if (section == "function"  && peek("'function"))  p_function(nn);
			else    break;
	}

	void p_type(Node& p) {
		require("'type @identifier endl");
		namecollision(presults.at(0));
			ctypename = presults.at(0);
			types[ctypename] = { ctypename };
		Node& nn = p.pushlist();
			nn.pushtokens({ "type", ctypename });
		nextline();
		// type members
		while (!eof()) {
			string name, type;
			if      (expect("endl"))  { nextline();  continue; }
			else if (expect("'dim @identifier @identifier endl"))  name = presults.at(1), type = presults.at(0);
			else if (expect("'dim @identifier endl"))  name = presults.at(0), type = "int";
			else    break;
			if (type == "integer")  type = "int";
				typecheck(type), namecollision(name);
				types.at(ctypename).members[name] = { name, type };  // save type member
			nn.pushlist().pushtokens({ "dim", name, type });
			nextline();
		}
		// end type
		ctypename = "";
		require("'end 'type endl"), nextline();
	}
	
	void p_dim(Node& p) {
		string name, type;
		if      (expect("'dim @identifier @identifier endl"))  name = presults.at(1), type = presults.at(0);
		else if (require("'dim @identifier endl"))  name = presults.at(0), type = "int";
		if (type == "integer")  type = "int";
		if (type != "int")  error2("TODO: non-int types");
			typecheck(type), namecollision(name);
			if    (!cfuncname.length())  globals[name] = { name, type };
			else  functions.at(cfuncname).locals[name] = { name, type };
		p.pushlist().pushtokens({ "dim", name, type });
		nextline();
	}

	void p_function(Node& p) {
		require("'function @identifier '(");
		namecollision(presults.at(0));
			cfuncname = presults.at(0);
			functions[cfuncname] = { cfuncname };
		auto& nn = p.pushlist();
			nn.pushtokens({ "function", cfuncname });
		// arguments
		auto& args = nn.pushlist();
			args.pushtoken("args");
		while (!eol()) {
			string name, type;
			if (!expect("@identifier @identifier"))  break;
			name = presults.at(1), type = presults.at(0);
			if (type != "int")  error2("TODO: non-int types");
				typecheck(type), namecollision(name);
				// if (functions[cfuncname].args.count(name))  error();  // additional name collision
				functions[cfuncname].args[name] = { name, type };  // save argument
				args.pushlist().pushtokens({ "dim", name, type });
			if (peek("')"))  break;
			require("',");
		}
		require("') endl"), nextline();
		// local dims
		auto& dims = nn.pushlist();
			dims.pushtokens({ "dimlocal" });
		while (!eof())
			if      (expect("'endl")) ;
			else if (peek("'dim"))  p_dim(dims);
			else    break;
		// block
		p_block(nn);
		// end function
		cfuncname = "";
		require("'end 'function endl"), nextline();
	}

	void p_block(Node& p) {
		auto& nn = p.pushlist();
			nn.pushtoken("block");
		while (!eof())
			if      (expect("endl"))   nextline();  // empty line
			else if (peek("'end"))     break;  // block end statement
			else if (peek("'else"))    break;  // block end statement
			else if (peek("'print"))   p_print(nn);
			// else if (peek("'input"))   p_input();
			else if (peek("'return"))  p_return(nn);
			else if (peek("'if"))      p_if(nn);
			else if (peek("'let"))     p_let(nn);
			else if (peek("'call"))    p_call(nn);
			else    error();
	}

	void p_print(Node& p) {
		require("'print");
		auto& l = p.pushlist();
			l.pushtoken("print");
		while (!eol())
			if      (peek("endl"))  break;
			else if (expect("',"))  l.push(Node::Literal(" "));
			else if (expect("';"))  l.push(Node::Literal("\t"));
			else if (expect("@literal"))  l.push(Node::Literal( presults.at(0) ));
			// TODO: string / object expression
			else {
				auto& ll = l.pushlist();
					ll.pushtoken("int_to_string");
					p_expr(ll);
			}
		// l.push(Node::Literal("\n"));
		require("endl"), nextline();
	}

	void p_return(Node& p) {
		require("'return");
		auto& l = p.pushlist();
			l.pushtoken("return");
		if    (expect("endl"))  l.pushtoken("0");
		else  p_expr(l);
		require("endl"), nextline();
	}

	void p_if(Node& p) {
		require("'if");
		auto& l = p.pushlist();
			l.pushtoken("if");
		// first comparison
		p_expr(l), require("endl"), nextline(), p_block(l);
		// else-if statements
		if (expect("'else 'if"))
			p_expr(l), require("endl"), nextline(), p_block(l);
		// last else
		if (expect("'else endl"))
			l.pushtoken("true"), nextline(), p_block(l);
		// block end
		require("'end 'if endl"), nextline();
	}

	void p_let(Node& p) {
		require("'let");
		p_varpath_set(p);
		auto& path = p.back();
		require("'=");
		p_expr(path), require("endl"), nextline();
	}

	void p_call(Node& p) {
		require("'call");
		p_expr_call(p);
		require("endl"), nextline();
	}



	// --- Expressions --- 

	void p_expr(Node& p) {
		p_expr_or(p);
	}

	void p_expr_or(Node& p) {
		p_expr_and(p);
		if (expect("'| '|")) {
			auto lhs = p.pop();
			auto& l  = p.pushlist();
				l.pushtoken("or");
				l.push(lhs);
			p_expr_or(l);  // parse rhs
		}
	}

	void p_expr_and(Node& p) {
		p_expr_comp(p);
		if (expect("'& '&")) {
			auto lhs = p.pop();
			auto& l  = p.pushlist();
				l.pushtoken("and");
				l.push(lhs);
			p_expr_and(l);  // parse rhs
		}
	}

	void p_expr_comp(Node& p) {
		p_expr_add(p);
		if (expect("@'= @'=") || expect("@'! @'=") || expect("@'<") || expect("@'>") || expect("@'< @'=") || expect("@'> @'=")) {
			string op = presults.at(0) + (presults.size() > 1 ? presults.at(1) : "");
			auto lhs = p.pop();
			auto& l  = p.pushlist();
				l.pushtoken("comp"+op);
				l.push(lhs);
			p_expr_add(l);  // parse rhs
		}
	}
	
	void p_expr_add(Node& p) {
		p_expr_mul(p);
		while (expect("@'+") || expect("@'-")) {
			auto lhs = p.pop();
			auto& l  = p.pushlist();
				l.pushtoken(presults.at(0) == "+" ? "add" : "sub");
				l.push(lhs);
			p_expr_mul(l);  // parse rhs
		}
	}

	void p_expr_mul(Node& p) {
		p_expr_atom(p);
		while (expect("@'*") || expect("@'/")) {
			auto lhs = p.pop();
			auto& l  = p.pushlist();
				l.pushtoken(presults.at(0) == "*" ? "mul" : "div");
				l.push(lhs);
			p_expr_atom(l);  // parse rhs
		}
	}
	
	void p_expr_atom(Node& p) {
		string type;
		if      (peek("identifier '("))  p_expr_call(p), type = "int";
		else if (peek("identifier"))     type = p_varpath(p);
		else if (expect("@integer"))     p.pushtoken(presults.at(0)), type = "int";
		else    error();
		if (type != "int")  error();
	}

	void p_expr_call(Node& p) {
		require("@identifier '(");
		auto fname = presults.at(0);
		auto& l = p.pushlist();
			l.pushtokens({ "call", fname });
		auto& args = l.pushlist();
		while (!eol() && !peek("')")) {
			p_expr(args);
			if (!expect("',"))  break;
		}
		require("')");
		if (functions[fname].args.size() != args.list.size())  error();
	}

	string p_varpath(Node& p) {
		// GET variables (TODO: messy)
		require("@identifier");
		string name = presults.at(0);
		auto& l = p.pushlist();
			// l.pushtoken("varpath");
		// local vars
		if (cfuncname.length() && functions[cfuncname].args.count(name)) {
			auto& d = functions[cfuncname].args[name];
			l.pushtokens({ "get_local", d.name, d.type });
			return d.type;
		}
		else if (cfuncname.length() && functions[cfuncname].locals.count(name)) {
			auto& d = functions[cfuncname].locals[name];
			l.pushtokens({ "get_local", d.name, d.type });
			return d.type;
		}
		// global vars
		else if (globals.count(name)) {
			auto&d = globals[name];
			l.pushtokens({ "get_global", d.name, d.type });
			return d.type;
		}
		else  return error(), "nil";
	}

	string p_varpath_set(Node& p) {
		// SET variables (TODO: messy)
		require("@identifier");
		string name = presults.at(0);
		auto& l = p.pushlist();
		// local vars
		if (cfuncname.length() && functions[cfuncname].args.count(name)) {
			auto& d = functions[cfuncname].args[name];
			l.pushtokens({ "set_local", d.name });
			return d.type;
		}
		else if (cfuncname.length() && functions[cfuncname].locals.count(name)) {
			auto& d = functions[cfuncname].locals[name];
			l.pushtokens({ "set_local", d.name });
			return d.type;
		}
		// global vars
		else if (globals.count(name)) {
			auto&d = globals[name];
			l.pushtokens({ "set_global", d.name });
			return d.type;
		}
		else  return error(), "nil";
	}



	// --- Etc. ---

	// overrides
	int require (const string& pattern) { return require (pattern, presults); }
	int require (const string& pattern, InputPattern::Results& results) {
		int len = peek(pattern, results);
		if (!len)  error();
		return pos += len, len;
	}
	


	// helpers
	int error() {
		throw DBParseError(lno);
	}
	int error2(const string& msg) {
		// Temporary WIP parser errors
		DBParseError d(lno);
		d.error_string += " :: " + msg;
		throw d;
	}
	void typecheck(const string& type) {
		if (type != "int" && type != "string" && types.count(type) == 0)  error();
	}
	void namecollision(const string& name) {
		// defined language collisions
		for (auto& k : BASIC_KEYWORDS)
			if (name == k)  error();
		// global collisions
		if (types.count(name))      error();
		if (functions.count(name))  error();
		// special type checking, based on parse state
		if      (ctypename.length()) { if (types.at(ctypename).members.count(name))  error(); }
		else if (cfuncname.length()) {
			if (functions.at(cfuncname).args.count(name))    error();
			if (functions.at(cfuncname).locals.count(name))  error();
		}
		else    { if (globals.count(name))  error(); }
	}
};