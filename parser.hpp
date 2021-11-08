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
	Node setup, teardown;
	map<string, Prog::Type> types;
	map<string, Prog::Dim>  globals;
	map<string, Prog::Func> functions;
	string cfuncname, ctypename;


	// --- Program structure parsing ---

	void p_program() {
		prog     = Node::CmdList("program");
		setup    = Node::CmdList("setup");
		teardown = Node::CmdList("teardown");
		p_section("type_defs", prog);
		p_section("dim_global", prog);
		prog.push(setup);
		prog.push(teardown);
		p_section("function_defs", prog);
	}

	void p_section(const string& section, Node& p) {
		Node& l = p.pushcmdlist(section);
		while (!eof())
			if      (expect("endl"))  nextline();
			else if (section == "type_defs"      && peek("'type"))      p_type(l);
			else if (section == "dim_global"     && peek("'dim"))       p_dim(l);
			else if (section == "function_defs"  && peek("'function"))  p_function(l);
			else    break;
	}

	void p_type(Node& p) {
		require("'type @identifier endl");
		namecollision(presults.at(0));
			ctypename        = presults.at(0);
			types[ctypename] = { ctypename };
		auto& l = p.pushlist();
			l.pushtokens({ "type", ctypename });
		nextline();
		// type members
		while (!eof()) {
			string name, type;
			if      (expect("endl"))  { nextline();  continue; }
			else if (expect("'dim @identifier @identifier endl"))  name = presults.at(1), type = presults.at(0);
			else if (expect("'dim @identifier endl"))  name = presults.at(0), type = "int";
			else    break;
			if (type == "integer")  type = "int";  // normalize type name
				typecheck(type), namecollision(name);
				types.at(ctypename).members[name] = { name, type };  // save type member
			l.pushlist().pushtokens({ "dim", name, type });
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
		if (type == "integer")  type = "int";  // normalize type name
			typecheck(type), namecollision(name);
			if    (!cfuncname.length())  globals[name] = { name, type };
			else  functions.at(cfuncname).locals[name] = { name, type };
		p.pushlist().pushtokens({ "dim", name, type });
		
		// TODO: cleanup
		if      (type == "int") ;
		else if (type == "float") ;
		else {
			if (type != "string")  error2("TODO: user types");
			string loc = cfuncname.length() ? "local" : "global";
			auto& ma = setup.pushlist();
				ma.pushtokens({ "malloc", loc, name, type });
			auto& fr = teardown.pushlist();
				fr.pushtokens({ "free",   loc, name, type });
		}

		// end dim
		nextline();
	}

	void p_function(Node& p) {
		require("'function @identifier '(");
		namecollision(presults.at(0));
			cfuncname            = presults.at(0);
			functions[cfuncname] = { cfuncname };
		// set up structure
		auto& fn = p.pushlist();
			fn.pushtokens({ "function", cfuncname });
		setup    = Node::CmdList("setup");
		teardown = Node::CmdList("teardown");
		// arguments
		auto& args = fn.pushcmdlist("args");
		while (!eol()) {
			string name, type;
			if (!expect("@identifier @identifier"))  break;
			name = presults.at(1), type = presults.at(0);
			if (type != "int")  error2("TODO: non-int types");
				typecheck(type), namecollision(name);
				functions[cfuncname].args[name] = { name, type };  // save argument
				args.pushlist().pushtokens({ "dim", name, type });
			if (peek("')"))  break;
			require("',");
		}
		require("') endl"), nextline();
		// local dims
		auto& dims = fn.pushcmdlist("dim_local");
		while (!eof())
			if      (expect("'endl")) ;
			else if (peek("'dim"))  p_dim(dims);
			else    break;
		// block
		fn.push(setup);
		p_block(fn);
		fn.push(teardown);
		// end function
		cfuncname = "";
		require("'end 'function endl"), nextline();
	}

	void p_block(Node& p) {
		auto& l = p.pushcmdlist("block");
		while (!eof())
			if      (expect("endl"))   nextline();  // empty line
			else if (peek("'end"))     break;  // block end statement
			else if (peek("'else"))    break;  // block end statement
			else if (peek("'print"))   p_print(l);
			// else if (peek("'input"))   p_input();
			else if (peek("'return"))  p_return(l);
			else if (peek("'if"))      p_if(l);
			else if (peek("'let"))     p_let(l);
			else if (peek("'call"))    p_call(l);
			else    error();
	}

	void p_print(Node& p) {
		require("'print");
		auto& l = p.pushcmdlist("print");
		while (!eol())
			if      (peek("endl"))  break;
			else if (expect("',"))  l.pushliteral(" ");
			else if (expect("';"))  l.pushliteral("\t");
			else {
				auto& ex = l.pushcmdlist("int_expr");
				auto  t  = p_anyexpr(ex);
				if      (t == "int") ;
				else if (t == "string")  ex.at(0).tok = "string_expr";
				else    error2("p_print");
			}
		// l.pushliteral("\n");
		require("endl"), nextline();
	}

	void p_return(Node& p) {
		require("'return");
		auto& l = p.pushcmdlist("return");
		if    (expect("endl"))  l.pushtoken("0");
		else  p_intexpr(l);
		require("endl"), nextline();
	}

	void p_if(Node& p) {
		require("'if");
		auto& l = p.pushcmdlist("if");
		// first comparison
		p_intexpr(l), require("endl"), nextline(), p_block(l);
		// else-if statements
		if (expect("'else 'if"))
			p_intexpr(l), require("endl"), nextline(), p_block(l);
		// last else
		if (expect("'else endl"))
			l.pushtoken("true"), nextline(), p_block(l);
		// block end
		require("'end 'if endl"), nextline();
	}

	void p_let(Node& p) {
		require("'let");
		auto t = p_varpath_set(p);
		require("'=");
		if (t == "int") {
			auto& l = p.back();
			p_intexpr(l), require("endl"), nextline();
		}
		else if (t == "string") {
			auto  lhs = p.pop();
			auto& l   = p.pushcmdlist("strcpy");
			l.push(lhs);
			p_strexpr(l), require("endl"), nextline();
		}
		else  error();
	}

	void p_call(Node& p) {
		require("'call");
		p_expr_call(p);
		require("endl"), nextline();
	}



	// --- Expressions --- 

	void   p_intexpr(Node& p) { p_expr_or(p) == "int"    || error2("p_intexpr"); }
	void   p_strexpr(Node& p) { p_expr_or(p) == "string" || error2("p_strexpr"); }
	string p_anyexpr(Node& p) { return p_expr_or(p); }

	string p_expr_or(Node& p) {
		auto t = p_expr_and(p);
		if (expect("'| '|")) {
			auto lhs = p.pop();
			auto& l  = p.pushlist();
				l.pushtoken("or");
				l.push(lhs);
			auto u = p_expr_or(l);  // parse rhs
			if (t != "int" || t != u)  error();
		}
		return t;
	}

	string p_expr_and(Node& p) {
		auto t = p_expr_comp(p);
		if (expect("'& '&")) {
			auto lhs = p.pop();
			auto& l  = p.pushlist();
				l.pushtoken("and");
				l.push(lhs);
			auto u = p_expr_and(l);  // parse rhs
			if (t != "int" || t != u)  error();
		}
		return t;
	}

	string p_expr_comp(Node& p) {
		auto t = p_expr_add(p);
		if (expect("@'= @'=") || expect("@'! @'=") || expect("@'<") || expect("@'>") || expect("@'< @'=") || expect("@'> @'=")) {
			string op = presults.at(0) + (presults.size() > 1 ? presults.at(1) : "");
			auto lhs = p.pop();
			auto& l  = p.pushlist();
				l.pushtoken("comp"+op);
				l.push(lhs);
			auto u = p_expr_add(l);  // parse rhs
			if (t != "int" || t != u)  error();
		}
		return t;
	}
	
	string p_expr_add(Node& p) {
		auto t = p_expr_mul(p);
		while (expect("@'+") || expect("@'-")) {
			auto lhs = p.pop();
			auto& l  = p.pushlist();
			if      (t == "int")  l.pushtoken(presults.at(0) == "+" ? "add" : "sub");
			else if (t == "string" && presults.at(0) == "+")  l.pushtoken("strcat");
			else    error();
			l.push(lhs);  // reappend lhs
			auto u = p_expr_mul(l);  // parse rhs
			if (t != u)  error();
		}
		return t;
	}

	string p_expr_mul(Node& p) {
		auto t = p_expr_atom(p);
		while (expect("@'*") || expect("@'/")) {
			auto lhs = p.pop();
			auto& l  = p.pushlist();
				l.pushtoken(presults.at(0) == "*" ? "mul" : "div");
				l.push(lhs);
			auto u = p_expr_atom(l);  // parse rhs
			if (t != "int" || t != u)  error();
		}
		return t;
	}
	
	string p_expr_atom(Node& p) {
		if      (peek("identifier '("))  return p_expr_call(p);
		else if (peek("identifier"))     return p_varpath(p);
		else if (expect("@integer"))     return p.pushtoken(presults.at(0)), "int";
		else if (expect("@literal"))     return p.pushliteral(presults.at(0)), "string";
		else    return error2("p_expr_atom"), "nil";
	}

	// string p_atom_type() {
	// 	if      (peek("identifier '("))  return "int";
	// 	else if (peek("@integer"))       return "int";
	// 	else if (peek("@literal"))       return "int";
	// 	else if (peek("identifier")) {
	// 		auto p = pos;
	// 		Node tmp(NT_LIST);
	// 		string t = p_varpath(tmp);
	// 		return pos = p, t;
	// 	}
	// 	else    return error(), "nil";
	// }

	string p_expr_call(Node& p) {
		require("@identifier '(");
		auto fname = presults.at(0);
		auto& l = p.pushlist();
			l.pushtokens({ "call", fname });
		auto& args = l.pushlist();
		while (!eol() && !peek("')")) {
			p_intexpr(args);
			if (!expect("',"))  break;
		}
		require("')");
		if (functions[fname].args.size() != args.list.size())  error();
		return "int";
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
		auto t  = p_varpath(p);
		auto& l = p.back();
		if      (t == "string") ;
		else if (l.cmd() == "get_local")   l.at(0).tok = "set_local";
		else if (l.cmd() == "get_global")  l.at(0).tok = "set_global";
		else    error2("p_varpath_set");
		return t;
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
		throw DBParseError(lno, currenttoken());
	}
	int error2(const string& msg) {
		// Temporary WIP parser errors
		DBParseError d(lno, currenttoken());
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