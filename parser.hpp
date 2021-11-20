// ----------------------------------------
// Basic parser main
// ----------------------------------------
#pragma once
#include "helpers.hpp"
#include "inputfile.hpp"
#include "stdlib.hpp"
#include <map>
using namespace std;


struct Prog {
	struct Dim  { string name, type; int index; };
	struct Type { string name; map<string, Dim> members; };
	struct Func { string name; map<string, Dim> args, locals; };
};

const vector<string> BASIC_KEYWORDS = {
	"int", "string",
	"type", "dim", "redim", "function", "end",
	"if", "while", "break", "continue",
};


struct Parser : InputFile {
	Node prog;
	Node setup, teardown;
	StdLib stdlib;
	map<string, Prog::Type> types;
	map<string, Prog::Dim>  globals;
	map<string, Prog::Func> functions;
	string cfuncname, ctypename;
	int flag_while = 0;  // parse flags


	// --- Program structure parsing ---

	void p_program() {
		prog     = Node::CmdList("program");
		setup    = Node::CmdList("setup");
		teardown = Node::CmdList("teardown");
		cfuncname = ctypename = "", flag_while = 0;
		p_section("type_defs", prog);
		p_section("dim_global", prog);
		prog.push(setup);
		prog.push(teardown);
		p_section("function_defs", prog);
		if (!eol())  error2("p_program");

		stdlib.appendto(prog.back());
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
		const string loc = cfuncname.length() ? "local" : "global";
		string name, type;
		if      (expect("'dim @identifier @identifier"))         name = presults.at(1), type = presults.at(0);
		else if (expect("'dim @identifier '[ '] @identifier"))   name = presults.at(1), type = presults.at(0) + "[]";
		else if (require("'dim @identifier"))                    name = presults.at(0), type = "int";
		if (type == "integer")  type = "int";  // normalize type name
		typecheck(type);  // checking
		pos--;  // put back one token (name)
		// loop and dim all
		while (!eof()) {
			// save dim info
			require("@identifier"), name = presults.at(0);
			namecollision(name);
			if    (!cfuncname.length())  globals[name] = { name, type };
			else  functions.at(cfuncname).locals[name] = { name, type };
			p.pushlist().pushtokens({ "dim", name, type });
			// initialisers (TODO: bit messy)
			if (type == "int") {
				// int initialisation
				if (expect("'=")) {
					auto& l = setup.pushlist();
					l.pushtokens({ "set_"+loc, name, type });
					p_intexpr(l);
				}
			}
			else if (is_arraytype(type)) {
				// array object assignment
				auto& ma = setup.pushlist();
				auto& fr = teardown.pushlist();
				ma.pushtokens({ "arrmalloc", loc, name, type }); //  ma.push(tmp.back()),
				fr.pushtokens({ "free",      loc, name, type });  // arrfree?
				if    (expect("'("))  p_intexpr(ma), require("')");
				else  ma.pushint(0);
			}
			else {
				// single object assignment
				auto& ma = setup.pushlist();
				auto& fr = teardown.pushlist();
				ma.pushtokens({ "malloc",    loc, name, type }),
				fr.pushtokens({ "free",      loc, name, type });
				// string assignment
				if (type == "string" && expect("'=")) {
					auto& l = setup.pushcmdlist("strcpy");
					auto& g = l.pushlist();
					g.pushtokens({ "get_"+loc, name, type });
					p_strexpr(l);
				}
			}
			// comma seperated list
			if (!expect("',"))  break;
		}
		// end dim
		require("endl"), nextline();
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
		string name, type;
		while (!eol()) {
			if      (expect("@identifier '[ '] @identifier"))   name = presults.at(1), type = presults.at(0) + "[]";
			else if (expect("@identifier @identifier"))         name = presults.at(1), type = presults.at(0);
			else    break;
			typecheck(type), namecollision(name);
				int argc = functions.at(cfuncname).args.size();
				functions.at(cfuncname).args[name] = { name, type, argc };  // save argument
				args.pushlist().pushtokens({ "dim", name, type });
			if (peek("')"))  break;
			require("',");
		}
		require("') endl"), nextline();
		// local dims
		auto& dims = fn.pushcmdlist("dim_local");
		while (!eof())
			if      (expect("endl"))  nextline();
			else if (peek("'dim"))    p_dim(dims);
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
			if      (expect("endl"))      nextline();  // empty line
			else if (peek("'end"))        break;  // block end statement
			else if (peek("'else"))       break;  // block end statement
			else if (peek("'print"))      p_print(l);
			else if (peek("'input"))      p_input(l);
			else if (peek("'return"))     p_return(l);
			else if (peek("'break"))      p_break(l);
			else if (peek("'continue"))   p_continue(l);
			else if (peek("'if"))         p_if(l);
			else if (peek("'while"))      p_while(l);
			else if (peek("'let"))        p_let(l);
			else if (peek("'redim"))      p_redim(l);
			else if (peek("'call"))       p_call(l);
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
				else if (t == "string" || t == "string$")  ex.at(0).tok = "string_expr";
				else    error2("p_print");
			}
		// l.pushliteral("\n");
		require("endl"), nextline();
	}

	void p_input(Node& p) {
		require("'input");
		auto& l = p.pushcmdlist("input");
		auto t = p_varpath_set(l);
		if (t != "string")  error();
		if    (expect("',"))  p_strexpr(l);  // user defined prompt
		else  l.pushliteral("> ");  // default prompt
		require("endl"), nextline();
	}

	void p_return(Node& p) {
		require("'return");
		auto& l = p.pushcmdlist("return");
		if    (expect("endl"))  l.pushtoken("0");
		else  p_intexpr(l);
		require("endl"), nextline();
	}

	void p_break(Node& p) {
		require("'break");
		if (!flag_while)  error2("p_break");
		auto& l = p.pushcmdlist("break");
		if (expect("@integer")) {
			int i = stoi(presults.at(0));  // specified break level
			if (i < 1 || i > flag_while)  error2("break level");
			l.pushint(i);
		}
		else  l.pushint(1);  // default break level (1)
		require("endl"), nextline();
	}

	void p_continue(Node& p) {
		require("'continue");
		if (!flag_while)  error2("p_continue");
		auto& l = p.pushcmdlist("continue");
		if (expect("@integer")) {
			int i = stoi(presults.at(0));  // specified continue level
			if (i < 1 || i > flag_while)  error2("continue level");
			l.pushint(i);
		}
		else  l.pushint(1);  // default continue level (1)
		require("endl"), nextline();
	}

	void p_if(Node& p) {
		require("'if");
		auto& l = p.pushcmdlist("if");
		p_intexpr(l), require("endl"), nextline(), p_block(l);  // first comparison
		while (expect("'else 'if"))
			p_intexpr(l), require("endl"), nextline(), p_block(l);  // else-if statements
		if (expect("'else endl"))
			l.pushtoken("true"), require("endl"), nextline(), p_block(l);  // last else
		require("'end 'if endl"), nextline();  // block end
	}

	void p_while(Node& p) {
		require("'while");
		flag_while++;
		auto& l = p.pushcmdlist("while");
		p_intexpr(l), nextline();
		p_block(l);
		require("'end 'while endl"), nextline();
		flag_while--;
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

	void p_redim(Node& p) {
		require("'redim");
		auto& l = p.pushcmdlist("redim");
		auto t = p_varpath(l);
		if (!is_arraytype(t))  error();
		require("',");
		p_intexpr(l);
		require("endl"), nextline();
	}

	void p_call(Node& p) {
		require("'call");
		p_expr_call(p);
		require("endl"), nextline();
	}



	// --- Expressions --- 

	void   p_intexpr(Node& p) { p_expr_or(p) == "int" || error2("p_intexpr"); }
	void   p_strexpr(Node& p) { auto t = p_expr_or(p);  t == "string" || t == "string$" || error2("p_strexpr"); }
	// void   p_sptexpr(Node& p) { p_expr_or(p) == "string" || error2("p_intexpr"); }
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
			if (t == "string")  t = "string$";  // normalise string
			if      (t == "int")  l.pushtoken("comp"+op);
			else if (t == "string$" && op == "==")  l.pushtoken("strcmp");
			else if (t == "string$" && op == "!=")  l.pushtoken("strncmp");
			else    error();
			l.push(lhs);  // reappend lhs
			auto u = p_expr_add(l);  // parse rhs
			if (u == "string")  u = "string$";  // normalise string
			if (t != u)  error();
			return "int";
		}
		return t;
	}
	
	string p_expr_add(Node& p) {
		auto t = p_expr_mul(p);
		while (expect("@'+") || expect("@'-")) {
			auto lhs = p.pop();
			auto& l  = p.pushlist();
			if (t == "string")  t = "string$";  // normalise string
			if      (t == "int")  l.pushtoken(presults.at(0) == "+" ? "add" : "sub");
			else if (t == "string$" && presults.at(0) == "+")  l.pushtoken("strcat");
			else    error();
			l.push(lhs);  // reappend lhs
			auto u = p_expr_mul(l);  // parse rhs
			if (u == "string")  u = "string$";  // normalise string
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
		if      (expect("'true"))         return p.pushtoken("true"),  "int";
		else if (expect("'false"))        return p.pushtoken("false"), "int";
		else if (peek("identifier '("))   return p_expr_call(p);
		else if (peek("identifier"))      return p_varpath(p);
		else if (expect("@integer"))      return p.pushint(presults.at(0)),     "int";
		else if (expect("@literal"))      return p.pushliteral(presults.at(0)), "string$";
		else    return error2("p_expr_atom"), "nil";
	}

	string p_varpath(Node& p) {
		string type = p_varpath_base(p);
		Node lhs;
		while (!eof())
			if       (peek("'."))  lhs = p.pop(),  type = p_varpath_prop(type, p),    p.back().push(lhs);
			else if  (peek("'["))  lhs = p.pop(),  type = p_varpath_arrpos(type, p),  p.back().push(lhs);
			else     break;
		return type;
	}
	
	string p_varpath_base(Node& p) {
		require("@identifier");
		auto name = presults.at(0);
		auto& l   = p.pushlist();
		// local vars
		if (cfuncname.length() && functions.at(cfuncname).args.count(name)) {
			auto& d = functions.at(cfuncname).args.at(name);
			l.pushtokens({ "get_local", d.name, d.type });
			return d.type;
		}
		else if (cfuncname.length() && functions.at(cfuncname).locals.count(name)) {
			auto& d = functions.at(cfuncname).locals.at(name);
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

	string p_varpath_prop(const string& type, Node& p) {
		require("'. @identifier");
		auto pname = presults.at(0);
		if (!types.at(type).members.count(pname))  error2("p_varpath_prop");
		auto& d = types.at(type).members.at(pname);
		auto& l = p.pushlist();
			l.pushtokens({ "get_property", type+"::"+d.name, d.type });
		return d.type;
	}
	
	string p_varpath_arrpos(const string& type, Node& p) {
		if (!is_arraytype(type))  error2("p_varpath_arrpos");
		require("'[");
		auto& l = p.pushcmdlist("get_arraypos");
			p_intexpr(l);
			l.pushtoken(basetype(type));
		require("']");
		return basetype(type);
	}

	string p_varpath_set(Node& p) {
		auto t  = p_varpath(p);
		auto& l = p.back();
		if      (t == "string") ;
		else if (l.cmd() == "get_local")     l.at(0).tok = "set_local";
		else if (l.cmd() == "get_global")    l.at(0).tok = "set_global";
		else if (l.cmd() == "get_property")  l.at(0).tok = "set_property";
		else if (l.cmd() == "get_arraypos")  l.at(0).tok = "set_arraypos";
		else    error2("p_varpath_set");
		return t;
	}

	void p_expr_calluser(Node& p) {
		require("@identifier '(");
		auto fname = presults.at(0);
		if (!functions.count(fname))  error2("missing function");
		auto& l = p.pushlist();
			l.pushtokens({ "call", fname });
		auto& args  = l.pushlist();
		int argc    = 0;
		while (!eol() && !peek("')")) {
			auto t = p_anyexpr(args);
			argc++;
			if (argat(fname, argc-1).type != t)  error2("p_expr_call argtype");
			if (!expect("',"))  break;
		}
		if (argc != functions.at(fname).args.size())  error2("p_expr_call argcount");
		require("')");
	}

	string p_expr_call(Node& p) {
		peek("@identifier");
		string fname = presults.at(0);
		if      (fname == "len")      std_len(p);
		else if (fname == "charat")   std_charat(p);
		// else if (fname == "push")   std_push(p);
		// else if (fname == "join")   std_join(p);
		else    p_expr_calluser(p);
		return "int";
	}



	// --- Std-library functions ---

	void std_len(Node& p) {
		require("'len '(");
		auto& l = p.pushcmdlist("sizeof");
		auto  t = p_anyexpr(l);
		if      (is_arraytype(t)) ;
		else if (t == "string")  l.at(0).tok = "strlen";
		else    error();
		require("')");
	}

	void std_charat(Node& p) {
		require("'charat '(");
		auto& l = p.pushcmdlist("charat");
		auto  t = p_anyexpr(l);
		if (t != "string")  error();
		require("',");
		p_intexpr(l);
		require("')");
	}

	// void std_push(Node& p) {
	// 	require("'push '(");
	// 	auto& l = p.pushlist();
	// 	l.pushtokens({ "call", "push" });
	// 	auto& args = l.pushlist();
	// 	auto t1 = p_anyexpr(args);
	// 	require("',");
	// 	auto t2 = p_anyexpr(args);
	// 	if (!is_arraytype(t1) || basetype(t1) != t2)  error();
	// 	require("')");
	// }

	// void std_join(Node& p) {
	// 	require("'join '(");
	// 	auto& l = p.pushlist();
	// 	l.pushtokens({ "call", "join" });
	// 	auto& args = l.pushlist();
	// 	auto t1 = p_anyexpr(args);
	// 	if (!is_arraytype(t1) || basetype(t1) != "string")  error();
	// 	require("',");
	// 	p_strexpr(args);
	// 	require("')");
	// }



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
		auto btype = basetype(type);
		if (btype != "int" && btype != "string" && types.count(btype) == 0)  error();
		if (ctypename == type)  error2("typecheck; circular definition");
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
	const Prog::Dim& argat(const string& fname, int index) {
		for (const auto& d : functions.at(fname).args)
			if (d.second.index == index)  return d.second;
		error2("missing argument "+to_string(index));
		throw DBError();
	}
};