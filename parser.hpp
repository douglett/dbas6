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
	struct Type { string name; vector<Dim> members; };
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
	void emit(const string& ln, const string& c="") { em.emit(ln, c); }
	void emlabel(const string& s) { em.label(s); }
	void dsym() { em.comment( "DSYM " + to_string(lno) + "   " + peekline() ); };



	// --- Program structure parsing ---

	void p_program() {
		cfuncname = ctypename = "";
		labelstack = {};
		flag_while = flag_ctrlcount = 0;
		em.header();
		p_section("type_defs");
		p_section("dim_global");
		emit("call main");
		emlabel( "SYSTEM_$" + string("teardown") );
		em.joinsub();
		emit("halt");
		p_section("function_defs");
		if (!eof())  error(ERR_EXPECTED_EOF);
		std_all();  // standard library
		usr_all();  // user types
	}

	void p_section(const string& section) {
		emlabel( "SYSTEM_$" + section );
		while (!eof())
			if      (expect("endl"))  nextline();
			else if (section == "type_defs"      && peek("'type"))      p_type();
			else if (section == "dim_global"     && peek("'dim"))       p_dim();
			else if (section == "function_defs"  && peek("'function"))  p_function();
			else    break;
	}



	// --- User types ---

	void p_type() {
		dsym();
		require("'type @identifier endl");
		namecollision(presults.at(0));
			ctypename        = presults.at(0);
			types[ctypename] = { ctypename };
		nextline();
		// type members
		while (!eof()) {
			string name, type;
			if      (expect("endl"))  { nextline();  continue; }
			else if (expect("'dim @identifier @identifier endl"))  name = presults.at(1), type = presults.at(0);
			else if (expect("'dim @identifier endl"))  name = presults.at(0), type = "int";
			else    break;
			typecheck(type), namecollision(name);
			types.at(ctypename).members.push_back({ name, type });  // save type member
			nextline();
		}
		// end type
		ctypename = "";
		require("'end 'type endl"), nextline();
	}

	void usr_all() {
		em.topcomment("------------------------");
		em.topcomment("---    User Types    ---");
		em.topcomment("------------------------");

		for (const auto& _t : types) {
			auto& t = _t.second;

			// type constructor
			emlabel(t.name + "_$construct");
			em.comment("construct () -> dest");
			emit("malloc0");
			for (auto& d : t.members)
				if      (d.type == "int")     emit("i 0",     usr_propsig(d)),  emit("mempush");
				else if (d.type == "string")  emit("malloc0", usr_propsig(d)),  emit("mempush");
				else    emit("call " + d.type + "_$construct", usr_propsig(d)),  emit("mempush");
			emit("ret");

			// type deconstructor
			emlabel(t.name + "_$deconstruct");
			em.comment("deconstruct (dest) -> dest");
			for (int i = t.members.size() - 1; i >= 0; i--) {
				auto& d = t.members[i];
				if      (d.type == "int")     emit("mempop", usr_propsig(d)),  emit("drop");
				else if (d.type == "string")  emit("mempop", usr_propsig(d)),  emit("free");
				else    emit("mempop", usr_propsig(d)),  emit("call " + d.type + "_$deconstruct"),  emit("free");
			}
			emit("ret");

			// type clone
			emlabel(t.name + "_$clone");
			em.comment("clone (dest, src) -> dest");
			emit("let dest");
			emit("set dest");
			emit("let src");
			emit("set src");
			emit("get_dest", "deconstruct dest");
			emit("call " + t.name + "_$deconstruct");
			emit("drop");
			for (int i = 0; i < t.members.size(); i++) {
				auto& d = t.members[i];
				emit("get src", usr_propsig(d));
				emit("i " + to_string(i));
				emit("memget");
				if      (d.type == "int")     emit("mempush");
				else if (d.type == "string")
					emit("stash"),
					emit("malloc0"),
					emit("unstash"),
					emit("memcopy"),
					emit("drop"),     // drop src member
					emit("mempush");  // push dest member
				else
					emit("stash"),
					emit("malloc0"),
					emit("unstash"),
					emit(d.type + "_$clone"),
					emit("drop"),     // drop src member
					emit("mempush");  // push dest member
			}
			emit("get dest");
			emit("ret");
		}
	}

	string usr_propsig(const Prog::Dim& dim) {
		return dim.name + " (" + dim.type + ")";
	}



	// --- Dim & Function ---

	void p_dim() {
		dsym();
		const int isglobal = cfuncname.length() == 0;
		const string get_cmd = (isglobal ? "get_global " : "get "), set_cmd = (isglobal ? "set_global " : "set ");
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
			emit( (isglobal ? "let_global " : "let ") + name, type );
			// -- initialisers (TODO: bit messy)
			// int init
			if (type == "int") {
				if (expect("'="))
					p_intexpr(),
					emit(set_cmd + name);
			}
			// string init
			else if (type == "string" || type == "int[]") {
				emit("malloc0");  // create string
				emit(set_cmd + name);
				em.emitsub(get_cmd + name);  // defer string teardown
				em.emitsub("free");
				if (expect("'=")) {
					emit(get_cmd + name);
					auto t = p_strexpr();
					emit("memcat");
					emit(t == "string$" ? "free" : "drop");
					emit("drop");
				}
			}
			// string[], user_type, user_type[]
			else if (type == "string[]" || types.count(type)) {
				if    (type == "string[]")  emit("malloc0", "create string[]");    // special case
				else  emit("call " + type + "_$construct", "create new " + type);  // general case
				emit(set_cmd + name);
				em.emitsub(get_cmd + name);
				em.emitsub("call " + type + "_$deconstruct");
				em.emitsub("free");
				if (expect("'=")) {
					emit(get_cmd + name);
					auto t = p_anyexpr();
					if (t != type)  error(ERR_UNMATCHED_TYPES);
					emit("call " + type + "_$clone");
					emit("drop");
				}
			}
			else  error(ERR_UNDEFINED_TYPE);
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
				emit("let " + name);
			if (peek("')"))  break;
			require("',");
		}
		require("') endl"), nextline();
		// emit arguments
		if (argc) {
			for (int i = argc-1; i >= 0; i--)
				emit( ("set " + argat(cfuncname, i).name), "argument "+to_string(i) );
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
		emit("get $rval");
		emit("ret");
		cfuncname = "";
	}



	// --- Block parsing

	void p_block() {
		while (!eof())
			if      (expect("endl"))      nextline();  // empty line
			else if (peek("'end"))        break;  // block end statement
			else if (peek("'else"))       break;  // block end statement
			else if (peek("'let"))        p_let();
			else if (peek("'call"))       p_call();
			else if (peek("'print"))      p_print();
			// else if (peek("'input"))      p_input(l);
			else if (peek("'return"))     p_return();
			else if (peek("'break"))      p_break();
			else if (peek("'continue"))   p_continue();
			else if (peek("'if"))         p_if();
			else if (peek("'while"))      p_while();
			else if (peek("'for"))        p_for();
			// arrays
			// else if (peek("'redim"))      p_redim(l);
			else if (peek("'push"))       p_push();
			else if (peek("'pop"))        p_pop();
			else    error(ERR_UNKNOWN_COMMAND);
	}

	void p_let() {
		dsym();
		require("'let");
		string type, getcmd, setcmd;
		type = p_varpath_set(getcmd, setcmd);
		require("'=");
		// int assign (expression)
		if (type == "int") {
			p_intexpr();
			emit(setcmd);
		}
		// string assign (expression)
		else if (type == "string" || type == "int[]") {
			emit(getcmd);
			auto t = p_strexpr();
			emit("memcopy");
			emit(t == "string$" ? "free" : "drop");
			emit("drop");
		}
		// string[], user_type, user_type[] ; assign (clone)
		else if (type == "string[]" || types.count(type)) {
			emit(getcmd);
			auto t = p_anyexpr();
			if (t != type)  error(ERR_UNMATCHED_TYPES);
			emit("call " + type + "_$clone");
			emit("drop");
		}
		// object assign (clone)
		else  error(ERR_UNEXPECTED_TYPE);
	}

	void p_call() {
		dsym();
		require("'call");
		p_expr_call();
		require("endl"), nextline();
		emit("drop", "discard return value");
	}

	void p_print() {
		dsym();
		require("'print");
		while (!eol())
			if      (peek("endl"))        break;
			else if (expect("',"))        emit("print_lit \" \"");
			else if (expect("';"))        emit("print_lit \"\t\"");
			else if (expect("@literal"))  emit("print_lit " + presults.at(0));
			else {
				auto t = p_anyexpr();
				if      (t == "int")      emit("print");
				else if (t == "string")   emit("print_str");
				else    error(ERR_UNEXPECTED_TYPE);
			}
		emit("print_lit \"\\n\"");
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
		if    (expect("endl"))  emit("i 0", "default return value");
		else  p_intexpr();
		require("endl"), nextline();
		emit("set $rval");
		emit("jump " + cfuncname + "_$teardown");
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
		emit( "jump " + labelstack.at(flag_while - break_level) + "end" );
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
		emit( "jump " + labelstack.at(flag_while - cont_level) + "start" );
	}

	void p_if() {
		require("'if");
		int cond = 0;
		string label = "$if_" + to_string(++flag_ctrlcount) + "_";
		emlabel(label + "start");
		dsym();
		// first comparison
		p_intexpr(), require("endl"), nextline();
		emit( "jumpifn " + label + to_string(cond+1), "first condition" );
		p_block();
		emit("jump " + label + "end");
		// else-if statements
		while (expect("'else 'if")) {
			emlabel(label + to_string(++cond));
			dsym();
			p_intexpr(), require("endl"), nextline();
			emit( ("jumpifn " + label+to_string(cond+1)), "condition "+to_string(cond) );
			p_block();
			emit("jump " + label + "end");
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
		emit("jumpifn " + label + "end");
		// main block
		p_block();
		// block end
		require("'end 'while endl"), nextline();
		emit("jump " + label + "start");  // loop
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
		emit("set " + name, "initialize");
		emlabel(label + "start");
		// to-value and bounds check
		emit("get " + name);
		require("'to"), p_intexpr();
		emit("lte");
		emit("jumpifn " + label + "end", "test iteration");
		// step (TODO)
		require("endl"), nextline();
		// main block
		p_block();
		// block end
		require("'end 'for endl"), nextline();
		emit("get " + name);  // iterate
		emit("i 1");
		emit("add");
		emit("set " + name);
		emit("jump " + label + "start");  // loop
		emlabel(label + "end");
		labelstack.pop_back();
		flag_while--;
	}

	void p_push() {
		dsym();
		require("'push");
		auto t = p_varpath_get();
		require("',");
		auto u = p_anyexpr();
		require("endl");
		if (!is_arraytype(t) && t != "string")  error(ERR_UNEXPECTED_TYPE);
		if ((t == "string" || t == "int[]") && u == "int")
			emit("mempush");
		else if (t == "string[]" && u == "string$")
			emit("mempush");
		else if (t == "string[]" && (u == "string" || u == "int[]"))
			emit("call string[]_$push");
		else  error(ERR_UNMATCHED_TYPES);
		// drop array ptr
		emit("drop");
	}

	void p_pop() {
		dsym();
		require("'pop");
		auto t = p_varpath_get();
		require("endl");
		if (t == "string" || t == "int[]")
			emit("mempop"),
			emit("drop");
		else if (t == "string[]")
			emit("mempop"),
			emit("free");
		// else if (types.count(t))
		// 	emit("mempop"),
		// 	emit(t + "_$deconstruct"),
		// 	emit("free");
		else  error(ERR_UNEXPECTED_TYPE);
		// drop array ptr
		emit("drop");
	}



	// --- Expressions ---

	void   p_intexpr() { p_expr_or() == "int"    || error(ERR_EXPECTED_INT); }
	string p_strexpr() { auto t = p_expr_or();  t == "string" || t == "string$" || t == "int[]" || error(ERR_EXPECTED_STRING);  return t; }
	string p_anyexpr() { return p_expr_or(); }
	// void   p_typeexpr(const string& type) { auto t = p_expr_or();  t == type || error(ERR_UNMATCHED_TYPES);  return t; }

	string p_expr_or() {
		auto t = p_expr_and();
		if (expect("'| '|")) {
			auto u = p_expr_or();
			if (t != "int" || t != u)  error(ERR_EXPECTED_INT);
			emit("or");
		}
		return t;
	}

	string p_expr_and() {
		auto t = p_expr_comp();
		if (expect("'& '&")) {
			auto u = p_expr_and();
			if (t != "int" || t != u)  error(ERR_EXPECTED_INT);
			emit("and");
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
				emit(opcode);
			}
			// string comparison
			else if (t == "string" || t == "string$" || t == "int[]") {
				if      (op == "==")  opcode = "streq";
				else if (op == "!=")  opcode = "strneq";
				else    error(ERR_STRING_OPERATOR_BAD);
				auto u = p_expr_add();
				// if (u != "string" && u != "string$")  error(ERR_EXPECTED_STRING);
				if (u != "string" && u != "string$" && u != "int[]")  error(ERR_UNMATCHED_TYPES);
				emit(opcode);
				emit("stash");  // manage strings on stack
				emit(u == "string$" ? "free" : "drop");
				emit(t == "string$" ? "free" : "drop");
				emit("unstash");
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
				emit(opcode);
			}
			// string concatenation
			else if (t == "string" || t == "string$" || t == "int[]") {
				if (op == "-")  error(ERR_STRING_OPERATOR_BAD);
				if (t == "string" || t == "int[]") {
					emit("stash");    // convert to string expression (on the stack)
					emit("malloc0");
					emit("unstash");
					emit("memcat");
					emit("drop");
					t = "string$";
				}
				auto u = p_expr_mul();
				// if (u != "string" && u != "string$")  error(ERR_EXPECTED_STRING);
				if (u != "string" && u != "string$" && u != "int[]")  error(ERR_UNMATCHED_TYPES);
				emit("memcat");
				emit(u == "string$" ? "free" : "drop");  // manage strings on stack
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
			emit(opcode);
		}
		return t;
	}

	string p_expr_atom() {
		if      (expect("'true"))         return emit("i 1", "true" ),    "int";
		else if (expect("'false"))        return emit("i 0", "false"),    "int";
		else if (expect("@integer"))      return emit("i " + presults.at(0)),  "int";
		else if (peek("identifier '("))   return p_expr_call();
		else if (peek("identifier"))      return p_varpath_get();
		else if (expect("@literal"))      return emit("malloc0"),  emit("memcat_lit " + presults.at(0)),  "string$";
		else if (eol())                   error(ERR_UNEXPECTED_EOL);
		return error(ERR_UNKNOWN_ATOM), "nil";
	}

	string p_varpath_get() {
		string rtype, getcmd, setcmd;
		rtype = p_varpath_set(getcmd, setcmd);
		emit(getcmd);
		return rtype;
	}

	string p_varpath_set(string& getcmd, string& setcmd) {
		// base
		auto& dim = p_vp_base();
		string rtype = dim.type;
		// array type
		if (peek("'["))
			emit( (dim.isglobal ? "get_global " : "get ") + dim.name ),
			rtype = p_vp_arrpos(rtype),
			getcmd = "memget",
			setcmd = "memset";
		else if (peek("'."))
			emit( (dim.isglobal ? "get_global " : "get ") + dim.name ),
			rtype = p_vp_objprop(rtype),
			getcmd = "memget",
			setcmd = "memset";
		else
			getcmd = (dim.isglobal ? "get_global " : "get ") + dim.name,
			setcmd = (dim.isglobal ? "set_global " : "set ") + dim.name;
		// return
		return rtype;
	}

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
		throw DBError();
	}
	
	string p_vp_arrpos(const string& type) {
		if (!is_arraytype(type) && type != "string")  error(ERR_EXPECTED_ARRAY);
		require("'[");
		p_intexpr();
		require("']");
		return type == "string" ? "int" : basetype(type);
	}

	string p_vp_objprop(const string& type) {
		if (!types.count(type))  error(ERR_EXPECTED_OBJECT);
		require("'. @identifier");
		auto prop = presults.at(0);
		const auto& tmem = types[type].members;
		for (int i = 0; i < tmem.size(); i++)
			if (tmem[i].name == prop) {
				emit("i " + to_string(i), usr_propsig(tmem[i]));
				return tmem[i].type;
			}
		// error
		error(ERR_UNDEFINED_PROPERTY);
		throw DBError();
	}

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
			emit("malloc0", "create temp string array"),
			emit("set $tmp");
		// arguments count
		while (!eol() && !peek("')")) {
			auto t = p_anyexpr();
			auto& def = argat(fname, argc);
			if (t == "string$" && def.type == "string") {  // special case - string expressions
				emit("cpstash", "save temp string");
				emit("get $tmp");
				emit("unstash");
				emit("mempush");
				emit("drop");
				strexprc++;
			}
			else if (def.type != t)
				error(ERR_ARGUMENT_TYPE_MISMATCH);    // argument type checking
			if (!expect("',"))  break;
			argc++;  // increment argument count
		}
		require("')");
		if (argc != functions.at(fname).args.size())  error(ERR_ARGUMENT_COUNT_MISMATCH);
		// do call
		emit("call " + fname);
		// cleanup
		if (argstrcount(fname)) {
			emit("get $tmp", "cleanup temp string array");
			for (int i = 0; i < strexprc; i++)
				emit("mempop"),
				emit("free");
			emit("free");
		}
		return "int";
	}



	// --- Std-library functions ---

	void std_all() {
		em.topcomment("------------------------");
		em.topcomment("--- Standard Library ---");
		em.topcomment("------------------------");
		std_stringarray();
	}

	void std_stringarray() {
		string label;

		// deconstructor (dest) -> dest
		label = "string[]_$deconstruct";
		emlabel(label);
		em.comment("deconstruct (dest) -> dest");  // function signiature
		emit("len");
		emit("i 0");
		emit("eq");
		emit("jumpif " + label + "_end");
		emit("mempop");
		emit("free");
		emit("jump " + label);
		emlabel(label + "_end");
		emit("ret");

		// push (dest, src) -> dest
		label = "string[]_$push";
		emlabel(label);
		em.comment("push (dest, src) -> dest");  // function signiature
		emit("stash");
		emit("malloc0");
		emit("unstash");
		emit("memcopy");
		emit("drop");
		emit("mempush");
		emit("ret");

		// clone (dest, src) -> dest
		label = "string[]_$clone";
		emlabel(label);
		em.comment("clone (dest, src) -> dest");  // function signiature
		// get arguments
		emit("let src", "get arguments");
		emit("set src");
		emit("let dest");
		emit("set dest");
		// iteration locals
		emit("let i", "iteration locals");
		emit("let src_len");
		emit("get src");
		emit("len");
		emit("set src_len");
		emit("drop");
		// deconstruct dest
		emit("get dest", "deconstruct dest");
		emit("call string[]_$deconstruct");
		emit("drop");
		// loop each item in src
		emlabel(label+"_loopstart");
		emit("get i");
		emit("get src_len");
		emit("lt");
		emit("jumpifn " + label + "_loopend");
		// clone string
		emit("get dest", "clone string");
		emit("malloc0");
		emit("get src");
		emit("get i");
		emit("memget");
		emit("memcat");
		emit("drop");
		emit("mempush");
		emit("drop");
		// next i
		emit("get i", "next i");
		emit("i 1");
		emit("add");
		emit("set i");
		emit("jump " + label + "_loopstart");
		// return
		emlabel(label + "_loopend");
		emit("get dest");
		emit("ret");
	}

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
		if (ctypename.length()) {
			for (const auto& d : types.at(ctypename).members)
				if (d.name == name)  error(ERR_ALREADY_DEFINED);
		}
		else if (cfuncname.length()) {
			if (functions.at(cfuncname).args.count(name))    error(ERR_ALREADY_DEFINED);
			if (functions.at(cfuncname).locals.count(name))  error(ERR_ALREADY_DEFINED);
		}
		else {
			if (globals.count(name))  error(ERR_ALREADY_DEFINED);
		}
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