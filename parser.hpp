// ----------------------------------------
// Basic parser main
// ----------------------------------------
#pragma once
#include "helpers.hpp"
#include "inputfile.hpp"
#include "emitter.hpp"
#include <map>
#include <ctime>
using namespace std;


struct Prog {
	struct Dim  { string name, type;  int isglobal, index; };
	struct Type { string name; map<string, Dim> members; };
	struct Func { string name; map<string, Dim> args, locals; };
};

const vector<string> BASIC_KEYWORDS = {
	"int", "string",
	"type", "dim", "redim", "function", "end",
	"if", "while", "break", "continue",
};


struct Parser : InputFile {
	map<string, Prog::Type> types;
	map<string, Prog::Dim>  globals;
	map<string, Prog::Func> functions;
	Emitter em;
	string cfuncname, ctypename;
	vector<string> labelstack;
	int flag_while = 0, flag_ctrlcount = 0;  // parse flags



	// --- Emitter shortcuts ---
	void emit(const vector<string>& vs, const string& c="") { em.emit(vs, c); }
	void emlabel(const string& s) { em.label(s); }
	void dsym() { em.comment( "DSYM " + to_string(lno) + "   " + peekline() ); };



	// --- Program structure parsing ---

	void p_program() {
		cfuncname = ctypename = "";
		labelstack = {};
		flag_while = flag_ctrlcount = 0;
		em.header();
		// p_section("type_defs");
		p_section("dim_global");
		emit({ "call", "main" });
		emlabel( "SYSTEM_$" + string("teardown") );
		em.joinsub();
		emit({ "halt" });
		p_section("function_defs");
		if (!eof())  error(ERR_EXPECTED_EOF);
	}

	void p_section(const string& section) {
		emlabel( "SYSTEM_$" + section );
		while (!eof())
			if      (expect("endl"))  nextline();
			// else if (section == "type_defs"      && peek("'type"))      p_type(l);
			else if (section == "dim_global"     && peek("'dim"))       p_dim();
			else if (section == "function_defs"  && peek("'function"))  p_function();
			else    break;
	}

	// void p_type(Node& p) {
	// 	require("'type @identifier endl");
	// 	namecollision(presults.at(0));
	// 		ctypename        = presults.at(0);
	// 		types[ctypename] = { ctypename };
	// 	auto& l = p.pushlist();
	// 		l.pushtokens({ "type", ctypename });
	// 	nextline();
	// 	// type members
	// 	while (!eof()) {
	// 		string name, type;
	// 		if      (expect("endl"))  { nextline();  continue; }
	// 		else if (expect("'dim @identifier @identifier endl"))  name = presults.at(1), type = presults.at(0);
	// 		else if (expect("'dim @identifier endl"))  name = presults.at(0), type = "int";
	// 		else    break;
	// 		if (type == "integer")  type = "int";  // normalize type name
	// 			typecheck(type), namecollision(name);
	// 			types.at(ctypename).members[name] = { name, type };  // save type member
	// 		l.pushlist().pushtokens({ "dim", name, type });
	// 		nextline();
	// 	}
	// 	// end type
	// 	ctypename = "";
	// 	require("'end 'type endl"), nextline();
	// }

	void p_dim() {
		dsym();
		const int isglobal = cfuncname.length() == 0;
		const string get_cmd = (isglobal ? "get_global" : "get"), set_cmd = (isglobal ? "set_global" : "set");
		string name, type;
		if      (expect("'dim @identifier @identifier"))         name = presults.at(1), type = presults.at(0);
		else if (expect("'dim @identifier '[ '] @identifier"))   name = presults.at(1), type = presults.at(0) + "[]";
		else if (require("'dim @identifier"))                    name = presults.at(0), type = "int";
		typecheck(type);  // checking
		pos--;  // put back one token (name)
		//
		// loop and dim all
		while (!eof()) {
			// -- save dim info
			require("@identifier"), name = presults.at(0);
			namecollision(name);
			if    (isglobal)  globals[name] = { name, type, .isglobal=isglobal };
			else  functions.at(cfuncname).locals[name] = { name, type, .isglobal=isglobal };
			emit({ (isglobal ? "let_global" : "let"), name }, type);
			// -- initialisers (TODO: bit messy)
			// int init
			if (type == "int") {
				if (expect("'="))
					p_intexpr(),
					emit({ set_cmd, name });
			}
			// string init
			else if (type == "string" || type == "int[]") {
				emit({ "malloc0" });  // create string
				emit({ set_cmd, name });
				em.emitsub({ get_cmd, name });  // defer string teardown
				em.emitsub({ "free" });
				if (expect("'=")) {
					emit({ get_cmd, name });
					auto t = p_strexpr();
					emit({ "memcat" });
					emit({ t == "string$" ? "free" : "drop" });
					emit({ "drop" });
				}
			}
			// array object assignment
			// else if (is_arraytype(type)) {
			// 	auto& ma = setup.pushlist();
			// 	auto& fr = teardown.pushlist();
			// 	ma.pushtokens({ "arrmalloc", loc, name, type }); //  ma.push(tmp.back()),
			// 	fr.pushtokens({ "free",      loc, name, type });  // arrfree?
			// 	if    (expect("'("))  p_intexpr(ma), require("')");
			// 	else  ma.pushint(0);
			// }
			// else {
			// 	// single object assignment
			// 	auto& ma = setup.pushlist();
			// 	auto& fr = teardown.pushlist();
			// 	ma.pushtokens({ "malloc",    loc, name, type }),
			// 	fr.pushtokens({ "free",      loc, name, type });
			// 	// string assignment
			// 	if (type == "string" && expect("'=")) {
			// 		auto& l = setup.pushcmdlist("strcpy");
			// 		auto& g = l.pushlist();
			// 		g.pushtokens({ "get_"+loc, name, type });
			// 		p_strexpr(l);
			// 	}
			// }
			// comma seperated list
			if (!expect("',"))  break;
		}
		// end dim
		require("endl"), nextline();
	}

	void p_function() {
		require("'function @identifier '(");
		namecollision(presults.at(0));
			cfuncname            = presults.at(0);
			functions[cfuncname] = { cfuncname };
			emlabel(cfuncname);
			dsym();
		// parse arguments
		string name, type;
		int argc = 0;
		while (!eol()) {
			if      (expect("@identifier '[ '] @identifier"))   name = presults.at(1), type = presults.at(0) + "[]";
			else if (expect("@identifier @identifier"))         name = presults.at(1), type = presults.at(0);
			else    break;
			typecheck(type), namecollision(name);
				functions.at(cfuncname).args[name] = { name, type, .isglobal=0, .index=argc };  // save argument
				argc++;  // increment args count
				emit({ "let", name });
			if (peek("')"))  break;
			require("',");
		}
		require("') endl"), nextline();
		// emit arguments
		if (argc) {
			for (int i = argc-1; i >= 0; i--)
				emit({ "set", argat(cfuncname, i).name }, "argument "+to_string(i));
		}
		// local dims
		while (!eof())
			if      (expect("endl"))  nextline();
			else if (peek("'dim"))    p_dim();
			else    break;
		// block
		p_block();
		// end function
		require("'end 'function endl"), nextline();
		// function teardown
		emlabel(cfuncname + "_$teardown");
		em.comment("function teardown");
		em.joinsub();
		emit({ "get", "$rval" });
		emit({ "ret" });
		cfuncname = "";
	}

	void p_block() {
		while (!eof())
			if      (expect("endl"))      nextline();  // empty line
			else if (peek("'end"))        break;  // block end statement
			else if (peek("'else"))       break;  // block end statement
			else if (peek("'let"))        p_let();
			else if (peek("'call"))       p_call();
			else if (peek("'print"))      p_print();
			// else if (peek("'input"))      p_input(l);
			// else if (peek("'redim"))      p_redim(l);
			else if (peek("'return"))     p_return();
			else if (peek("'break"))      p_break();
			else if (peek("'continue"))   p_continue();
			else if (peek("'if"))         p_if();
			else if (peek("'while"))      p_while();
			else if (peek("'for"))        p_for();
			else    error(ERR_UNKNOWN_COMMAND);
	}

	void p_let() {
		dsym();
		require("'let");
		auto& def = p_vp_base();
		require("'=");
		// int assign (expression)
		if (def.type == "int") {
			p_intexpr();
			emit({ (def.isglobal ? "set_global" : "set"), def.name });
		}
		// string assign (expression)
		else if (def.type == "string" || def.type == "int[]") {
			emit({ (def.isglobal ? "get_global" : "get"), def.name });
			auto t = p_strexpr();
			emit({ "memcopy" });
			emit({ t == "string$" ? "free" : "drop" });
			emit({ "drop" });
		}
		// object assign (clone)
		else  error(ERR_UNEXPECTED_TYPE);
	}

	void p_call() {
		dsym();
		require("'call");
		p_expr_call();
		require("endl"), nextline();
		emit({ "drop" }, "discard return value");
	}

	void p_print() {
		dsym();
		require("'print");
		while (!eol())
			if      (peek("endl"))        break;
			else if (expect("',"))        emit({ "print_lit", "\" \"" });
			else if (expect("';"))        emit({ "print_lit", "\"\t\"" });
			else if (expect("@literal"))  emit({ "print_lit", presults.at(0) });
			else {
				auto t = p_anyexpr();
				if      (t == "int")      emit({ "print" });
				else if (t == "string")   emit({ "print_str" });
				else    error(ERR_UNEXPECTED_TYPE);
			}
		emit({ "print_lit", "\"\\n\"" });
		require("endl"), nextline();
	}

	// void p_input(Node& p) {
	// 	require("'input");
	// 	auto& l = p.pushcmdlist("input");
	// 	auto t = p_varpath_set(l);
	// 	if (t != "string")  error();
	// 	if    (expect("',"))  p_strexpr(l);  // user defined prompt
	// 	else  l.pushliteral("> ");  // default prompt
	// 	require("endl"), nextline();
	// }

	// void p_redim(Node& p) {
	// 	require("'redim");
	// 	auto& l = p.pushcmdlist("redim");
	// 	auto t = p_varpath(l);
	// 	if (!is_arraytype(t))  error();
	// 	require("',");
	// 	p_intexpr(l);
	// 	require("endl"), nextline();
	// }

	void p_return() {
		dsym();
		require("'return");
		if    (expect("endl"))  emit({ "i", "0" }, "default return value");
		else  p_intexpr();
		require("endl"), nextline();
		emit({ "set", "$rval" });
		emit({ "jump", cfuncname + "_$teardown" });
	}

	void p_break() {
		dsym();
		require("'break");
		if (!flag_while)  error(ERR_BREAK_OUTSIDE_LOOP);
		int break_level = 1;
		if (expect("@integer")) {
			break_level = stoi(presults.at(0));  // specified break level
			if (break_level < 1 || break_level > flag_while)  error(ERR_BREAK_LEVEL_BAD);
		}
		require("endl"), nextline();
		emit({ "jump", labelstack.at(flag_while - break_level) + "end" });
	}

	void p_continue() {
		dsym();
		require("'continue");
		if (!flag_while)  error(ERR_CONTINUE_OUTSIDE_LOOP);
		int cont_level = 1;
		if (expect("@integer")) {
			int cont_level = stoi(presults.at(0));  // specified continue level
			if (cont_level < 1 || cont_level > flag_while)  error(ERR_CONTINUE_LEVEL_BAD);
		}
		require("endl"), nextline();
		emit({ "jump", labelstack.at(flag_while - cont_level) + "start" });
	}

	void p_if() {
		require("'if");
		int cond = 0;
		string label = "$if_" + to_string(++flag_ctrlcount) + "_";
		emlabel(label + "start");
		dsym();
		// first comparison
		p_intexpr(), require("endl"), nextline();
		emit({ "jumpifn", label + to_string(cond+1) }, "first condition");
		p_block();
		emit({ "jump", label + "end" });
		// else-if statements
		while (expect("'else 'if")) {
			emlabel(label + to_string(++cond));
			dsym();
			p_intexpr(), require("endl"), nextline();
			emit({ "jumpifn", label+to_string(cond+1) }, "condition "+to_string(cond));
			p_block();
			emit({ "jump", label + "end" });
		}
		// if all conditions fail, we end up here
		emlabel(label + to_string(++cond));
		// last else, guaranteed execution
		if (expect("'else endl")) {
			dsym();
			nextline();
			p_block();
		}
		// block end
		require("'end 'if endl"), nextline();
		emlabel(label + "end");
	}

	void p_while() {
		require("'while");
		flag_while++;
		string label = "$while_" + to_string(++flag_ctrlcount) + "_";
		labelstack.push_back(label);
		emlabel(label + "start");
		dsym();
		// comparison
		p_intexpr(), require("endl"), nextline();
		emit({ "jumpifn", label + "end" });
		// main block
		p_block();
		// block end
		require("'end 'while endl"), nextline();
		emit({ "jump", label + "start" });  // loop
		emlabel(label + "end");
		labelstack.pop_back();
		flag_while--;
	}

	void p_for() {
		require("'for");
		flag_while++;  // lets say this is a while loop of sorts
		string name, label = "$for_" + to_string(++flag_ctrlcount) + "_";
		labelstack.push_back(label);
		emlabel(label + "pre_start");
		dsym();
		// from-value
		require("@identifier '="), name = presults.at(0), p_intexpr();
		emit({ "set", name }, "initialize");
		emlabel(label + "start");
		// to-value and bounds check
		emit({ "get", name });
		require("'to"), p_intexpr();
		emit({ "lte" });
		emit({ "jumpifn", label + "end" }, "test iteration");
		// step (TODO)
		require("endl"), nextline();
		// main block
		p_block();
		// block end
		require("'end 'for endl"), nextline();
		emit({ "get", name });  // iterate
		emit({ "i", "1" });
		emit({ "add" });
		emit({ "set", name });
		emit({ "jump", label + "start" });  // loop
		emlabel(label + "end");
		labelstack.pop_back();
		flag_while--;
	}



	// --- Expressions ---

	void   p_intexpr() { p_expr_or() == "int"    || error(ERR_EXPECTED_INT); }
	string p_strexpr() { auto t = p_expr_or();  t == "string" || t == "string$" || t == "int[]" || error(ERR_EXPECTED_STRING);  return t; }
	string p_anyexpr() { return p_expr_or(); }

	string p_expr_or() {
		auto t = p_expr_and();
		if (expect("'| '|")) {
			auto u = p_expr_or();
			if (t != "int" || t != u)  error(ERR_EXPECTED_INT);
			emit({ "or" });
		}
		return t;
	}

	string p_expr_and() {
		auto t = p_expr_comp();
		if (expect("'& '&")) {
			auto u = p_expr_and();
			if (t != "int" || t != u)  error(ERR_EXPECTED_INT);
			emit({ "and" });
		}
		return t;
	}

	string p_expr_comp() {
		auto t = p_expr_add();
		if (expect("@'= @'=") || expect("@'! @'=") || expect("@'< @'=") || expect("@'> @'=") || expect("@'<") || expect("@'>")) {
			string  opcode,  op = (presults.at(0) + (presults.size() > 1 ? presults.at(1) : ""));
			// int comparison
			if (t == "int") {
				if (op == "==")  opcode = "eq";
				if (op == "!=")  opcode = "neq";
				if (op == "<" )  opcode = "lt";
				if (op == ">" )  opcode = "gt";
				if (op == "<=")  opcode = "lte";
				if (op == ">=")  opcode = "gte";
				auto u = p_expr_add();
				// if (u != "int")  error(ERR_EXPECTED_INT);
				if (u != "int")  error(ERR_UNMATCHED_TYPES);
				emit({ opcode });
			}
			// string comparison
			else if (t == "string" || t == "string$" || t == "int[]") {
				if      (op == "==")  opcode = "streq";
				else if (op == "!=")  opcode = "strneq";
				else    error(ERR_STRING_OPERATOR_BAD);
				auto u = p_expr_add();
				// if (u != "string" && u != "string$")  error(ERR_EXPECTED_STRING);
				if (u != "string" && u != "string$" && u != "int[]")  error(ERR_UNMATCHED_TYPES);
				emit({ opcode });
				emit({ "stash" });  // manage strings on stack
				emit({ u == "string$" ? "free" : "drop" });
				emit({ t == "string$" ? "free" : "drop" });
				emit({ "unstash" });
			}
			else  error(ERR_OBJECT_OPERATOR_BAD);  // unexpected type for this operator
			return "int";  // ok
		}
		return t;
	}

	string p_expr_add() {
		auto t = p_expr_mul();
		while (expect("@'+") || expect("@'-")) {
			string  opcode,  op = presults.at(0);
			// int addition
			if (t == "int") {
				opcode = (op == "+" ? "add" : "sub");
				auto u = p_expr_mul();
				// if (u != "int")  error(ERR_EXPECTED_INT);
				if (u != "int")  error(ERR_UNMATCHED_TYPES);
				emit({ opcode });
			}
			// string concatenation
			else if (t == "string" || t == "string$" || t == "int[]") {
				if (op == "-")  error(ERR_STRING_OPERATOR_BAD);
				if (t == "string" || t == "int[]") {
					emit({ "stash" });    // convert to string expression (on the stack)
					emit({ "malloc0" });
					emit({ "unstash" });
					emit({ "memcat" });
					emit({ "drop" });
					t = "string$";
				}
				auto u = p_expr_mul();
				// if (u != "string" && u != "string$")  error(ERR_EXPECTED_STRING);
				if (u != "string" && u != "string$" && u != "int[]")  error(ERR_UNMATCHED_TYPES);
				emit({ "memcat" });
				emit({ u == "string$" ? "free" : "drop" });  // manage strings on stack
			}
			else  error(ERR_OBJECT_OPERATOR_BAD);  // unexpected type for this operator
		}
		return t;
	}

	string p_expr_mul() {
		auto t = p_expr_atom();
		while (expect("@'*") || expect("@'/")) {
			string opcode = presults.at(0) == "*" ? "mul" : "div";
			auto u = p_expr_atom();
			if (t != "int" || t != u)  error(ERR_EXPECTED_INT);
			emit({ opcode });
		}
		return t;
	}

	string p_expr_atom() {
		if      (expect("'true"))         return emit({ "i", "1" }, "true" ),    "int";
		else if (expect("'false"))        return emit({ "i", "0" }, "false"),    "int";
		else if (expect("@integer"))      return emit({ "i", presults.at(0) }),  "int";
		else if (peek("identifier '("))   return p_expr_call();
		else if (peek("identifier"))      return p_varpath();
		else if (expect("@literal"))      return emit({ "malloc0" }),  emit({ "memcat_lit", presults.at(0) }),  "string$";
		else if (eol())                   error(ERR_UNEXPECTED_EOL);
		return error(ERR_UNKNOWN_ATOM), "nil";
	}

	string p_varpath() {
		// string type = p_varpath_base();
		// while (!eof())
		// 	if       (peek("'."))  lhs = p.pop(),  type = p_varpath_prop(type, p),    p.back().push(lhs);
		// 	else if  (peek("'["))  lhs = p.pop(),  type = p_varpath_arrpos(type, p),  p.back().push(lhs);
		// 	else     break;
		// return type;

		auto& def = p_vp_base();
		emit({ (def.isglobal ? "get_global" : "get"), def.name });
		return def.type;
	}

	// string p_varpath_base() {
	// 	require("@identifier");
	// 	auto name = presults.at(0);
	// 	// local vars
	// 	if (cfuncname.length() && functions.at(cfuncname).args.count(name)) {
	// 		auto& d = functions.at(cfuncname).args.at(name);
	// 		emit({ "get", name });
	// 		return d.type;
	// 	}
	// 	else if (cfuncname.length() && functions.at(cfuncname).locals.count(name)) {
	// 		auto& d = functions.at(cfuncname).locals.at(name);
	// 		emit({ "get", name });
	// 		return d.type;
	// 	}
	// 	// global vars
	// 	else if (globals.count(name)) {
	// 		auto&d = globals[name];
	// 		emit({ "get_global", name });
	// 		return d.type;
	// 	}
	// 	else  return error(ERR_UNDEFINED_VARIABLE), "nil";
	// }

	const Prog::Dim& p_vp_base() {
		require("@identifier");
		auto name = presults.at(0);
		// local vars
		if (cfuncname.length() && functions.at(cfuncname).args.count(name))
			return functions[cfuncname].args[name];
		else if (cfuncname.length() && functions.at(cfuncname).locals.count(name))
			return functions[cfuncname].locals[name];
		// global vars
		else if (globals.count(name))
			return globals[name];
		// error
		error(ERR_UNDEFINED_VARIABLE);
		throw std::exception();
	}

	// string p_varpath_prop(const string& type, Node& p) {
	// 	require("'. @identifier");
	// 	auto pname = presults.at(0);
	// 	if (!types.at(type).members.count(pname))  error2("p_varpath_prop");
	// 	auto& d = types.at(type).members.at(pname);
	// 	auto& l = p.pushlist();
	// 		l.pushtokens({ "get_property", type+"::"+d.name, d.type });
	// 	return d.type;
	// }

	// string p_varpath_arrpos(const string& type, Node& p) {
	// 	if (!is_arraytype(type))  error2("p_varpath_arrpos");
	// 	require("'[");
	// 	auto& l = p.pushcmdlist("get_arraypos");
	// 		p_intexpr(l);
	// 		l.pushtoken(basetype(type));
	// 	require("']");
	// 	return basetype(type);
	// }

	// string p_varpath_set(Node& p) {
	// 	auto t  = p_varpath(p);
	// 	auto& l = p.back();
	// 	if      (t == "string") ;
	// 	else if (l.cmd() == "get_local")     l.at(0).tok = "set_local";
	// 	else if (l.cmd() == "get_global")    l.at(0).tok = "set_global";
	// 	else if (l.cmd() == "get_property")  l.at(0).tok = "set_property";
	// 	else if (l.cmd() == "get_arraypos")  l.at(0).tok = "set_arraypos";
	// 	else    error2("p_varpath_set");
	// 	return t;
	// }

	string p_expr_call() {
		// peek("@identifier");
		// string fname = presults.at(0);
		// if      (fname == "len")      std_len(p);
		// else if (fname == "charat")   std_charat(p);
		// else if (fname == "substr")   std_substr(p);
		// // else if (fname == "push")   std_push(p);
		// // else if (fname == "join")   std_join(p);
		// else    p_expr_calluser(p);
		// return "int";
		if (cfuncname.length() == 0)  error(ERR_CALL_FUNCTION_GLOBAL);
		return p_expr_call_user();
	}

	string p_expr_call_user() {
		require("@identifier '(");
		auto fname = presults.at(0);
		if (!functions.count(fname))  error(ERR_UNDEFINED_FUNCTION);
		int argc = 0, strexprc = 0;
		// create temp string array (TODO: could optimise this)
		if (argstrcount(fname))
			emit({ "malloc0" }, "create temp string array"),
			emit({ "set", "$tmp" });
		// arguments count
		while (!eol() && !peek("')")) {
			auto t = p_anyexpr();
			argc++;
			auto& def = argat(fname, argc-1);
			if (t == "string$" && def.type == "string") {  // special case - string expressions
				emit({ "cpstash" }, "save temp string");
				emit({ "get", "$tmp" });
				emit({ "unstash" });
				emit({ "mempush" });
				emit({ "drop" });
				strexprc++;
			}
			else if (def.type != t)
				error(ERR_ARGUMENT_TYPE_MISMATCH);    // argument type checking
			if (!expect("',"))  break;
		}
		require("')");
		if (argc != functions.at(fname).args.size())  error(ERR_ARGUMENT_COUNT_MISMATCH);
		// do call
		emit({ "call", fname });
		// cleanup
		if (argstrcount(fname)) {
			emit({ "get", "$tmp" }, "cleanup temp string array");
			for (int i = 0; i < strexprc; i++)
				emit({ "mempop" }),
				emit({ "free" });
			emit({ "free" });
		}
		return "int";
	}



	// --- Std-library functions ---

	// void std_len(Node& p) {
	// 	require("'len '(");
	// 	auto& l = p.pushcmdlist("sizeof");
	// 	auto  t = p_anyexpr(l);
	// 	if      (is_arraytype(t)) ;
	// 	else if (t == "string")  l.at(0).tok = "strlen";
	// 	else    error();
	// 	require("')");
	// }

	// void std_charat(Node& p) {
	// 	require("'charat '(");
	// 	auto& l = p.pushcmdlist("charat");
	// 	auto  t = p_anyexpr(l);
	// 	if (t != "string")  error();
	// 	require("',");
	// 	p_intexpr(l);
	// 	require("')");
	// }

	// void std_substr(Node& p) {
	// 	require("'substr '(");
	// 	auto& l = p.pushcmdlist("substr");
	// 	p_strexpr(l), require("',");  // src
	// 	p_strexpr(l), require("',");  // dest
	// 	p_intexpr(l), require("',");  // pos
	// 	p_intexpr(l), require("')");  // len
	// }

	// // void std_push(Node& p) {
	// // 	require("'push '(");
	// // 	auto& l = p.pushlist();
	// // 	l.pushtokens({ "call", "push" });
	// // 	auto& args = l.pushlist();
	// // 	auto t1 = p_anyexpr(args);
	// // 	require("',");
	// // 	auto t2 = p_anyexpr(args);
	// // 	if (!is_arraytype(t1) || basetype(t1) != t2)  error();
	// // 	require("')");
	// // }

	// // void std_join(Node& p) {
	// // 	require("'join '(");
	// // 	auto& l = p.pushlist();
	// // 	l.pushtokens({ "call", "join" });
	// // 	auto& args = l.pushlist();
	// // 	auto t1 = p_anyexpr(args);
	// // 	if (!is_arraytype(t1) || basetype(t1) != "string")  error();
	// // 	require("',");
	// // 	p_strexpr(args);
	// // 	require("')");
	// // }



	// --- Etc. ---

	// overrides
	int require (const string& pattern) { return require (pattern, presults); }
	int require (const string& pattern, InputPattern::Results& results) {
		int len = peek(pattern, results);
		if (!len)  error(ERR_SYNTAX_ERROR);
		return pos += len, len;
	}



	// helpers
	int error(DB_PARSE_ERROR err) {
		throw DBParseError(err, lno, currenttoken());
	}
	void typecheck(const string& type) {
		auto btype = basetype(type);
		if (btype != "int" && btype != "string" && types.count(btype) == 0)  error(ERR_UNDEFINED_TYPE);
		if (ctypename == type)  error(ERR_CIRCULAR_DEFINITION);
	}
	void namecollision(const string& name) {
		// defined language collisions
		for (auto& k : BASIC_KEYWORDS)
			if (name == k)  error(ERR_ALREADY_DEFINED);
		// global collisions
		if (types.count(name))      error(ERR_ALREADY_DEFINED);
		if (functions.count(name))  error(ERR_ALREADY_DEFINED);
		// special type checking, based on parse state
		if      (ctypename.length()) { if (types.at(ctypename).members.count(name))  error(ERR_ALREADY_DEFINED); }
		else if (cfuncname.length()) {
			if (functions.at(cfuncname).args.count(name))    error(ERR_ALREADY_DEFINED);
			if (functions.at(cfuncname).locals.count(name))  error(ERR_ALREADY_DEFINED);
		}
		else    { if (globals.count(name))  error(ERR_ALREADY_DEFINED); }
	}
	const Prog::Dim& argat(const string& fname, int index) {
		for (const auto& d : functions.at(fname).args)
			if (d.second.index == index)  return d.second;
		error(ERR_ARGUMENT_INDEX_MISSING);
		throw DBError();
	}
	int argstrcount(const string& fname) {
		int count = 0;
		for (const auto& d : functions.at(fname).args)
			if (d.second.type == "string")  count++;
		return count;
	}
};