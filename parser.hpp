// ----------------------------------------
// Basic parser main
// ----------------------------------------
#pragma once
#include "helpers.hpp"
#include "inputfile.hpp"
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
	vector<string> prog2;
	// vector<string> setup2, teardown2;
	map<string, Prog::Type> types;
	map<string, Prog::Dim>  globals;
	map<string, Prog::Func> functions;
	string cfuncname, ctypename;
	int flag_while = 0, flag_lastret = 0, flag_ctrlpoint = 0;  // parse flags


	
	// --- Emit asm ---
	
	const string COMMENT_SEP = "\t ; ";
	vector<string> while_labels;  // emit state
	
	void emit(const vector<string>& vs, const string& c="") {
		flag_lastret = (vs.size() && vs.at(0) == "ret");
		prog2.push_back( "\t" + join(vs) + (c.length() ? COMMENT_SEP + c : "") );
	}
	// void emit_at(int32_t pos, const vector<string>& vs, const string& c="") {
	// 	prog2.at(pos) = "\t" + join(vs) + (c.length() ? COMMENT_SEP + c : "");
	// }
	void emitl(const string& s, const string& c="") {
		prog2.push_back( s + ":" + (c.length() ? COMMENT_SEP + c : "") );
	}
	void emitc(const string& c) {
		prog2.push_back( "\t; " + c );
	}
	void dsym() {
		emitc("DSYM " + to_string(lno) + "   " + peekline());
	}



	// --- Program structure parsing ---

	void p_program() {
		prog2     = {};
		cfuncname = ctypename = "",  flag_while = flag_lastret = flag_ctrlpoint = 0;
		// p_section("type_defs");
		p_section("dim_global");
		emit({ "call", "main" });
		emit({ "halt" });
		// prog.push(setup);
		// prog.push(teardown);
		p_section("function_defs");
		if (!eof())  error(ERR_EXPECTED_EOF);
	}

	void p_section(const string& section) {
		emitl( "$" + section );
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
			if    (!cfuncname.length())  globals[name] = { name, type };
			else  functions.at(cfuncname).locals[name] = { name, type };
			emit({ (isglobal ? "let_global" : "let"), name }, type);
			// -- initialisers (TODO: bit messy)
			if (type == "int") {
				// int initialisation
				if (expect("'=")) {
					// auto& l = setup.pushlist();
					// l.pushtokens({ "set_"+loc, name, type });
					p_intexpr();
					emit({ (isglobal ? "set_global" : "set"), name });
				}
			}
			// else if (is_arraytype(type)) {
			// 	// array object assignment
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
			emitl(cfuncname);
			dsym();
		// set up structure
		// setup2 = teardown2 = {};
		// parse arguments
		string name, type;
		int argc = 0;
		while (!eol()) {
			if      (expect("@identifier '[ '] @identifier"))   name = presults.at(1), type = presults.at(0) + "[]";
			else if (expect("@identifier @identifier"))         name = presults.at(1), type = presults.at(0);
			else    break;
			typecheck(type), namecollision(name);
				functions.at(cfuncname).args[name] = { name, type, argc };  // save argument
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
		// emit setup here
		p_block();
		// emit teardown here
		// end function
		require("'end 'function endl"), nextline();
		if (!flag_lastret) {
			emit({ "i", "0" }, "default return value");
			emit({ "ret" });
		}
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
			else    error(ERR_UNKNOWN_COMMAND);
	}

	void p_let() {
		// require("'let");
		// auto t = p_varpath_set(p);
		// require("'=");
		// if (t == "int") {
		// 	auto& l = p.back();
		// 	p_intexpr(l), require("endl"), nextline();
		// }
		// else if (t == "string") {
		// 	auto  lhs = p.pop();
		// 	auto& l   = p.pushcmdlist("strcpy");
		// 	l.push(lhs);
		// 	p_strexpr(l), require("endl"), nextline();
		// }
		// else  error();

		dsym();
		require("'let @identifier '=");
		auto name = presults.at(0);
		p_intexpr();
		if (cfuncname.length() && functions.at(cfuncname).args.count(name))  emit({ "set", name });
		else if (cfuncname.length() && functions.at(cfuncname).locals.count(name))  emit({ "set", name });
		else if (globals.count(name))  emit({ "set_global", name });
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
			else    p_intexpr(), emit({ "print" });
			// else {
			// 	auto& ex = l.pushcmdlist("int_expr");
			// 	auto  t  = p_anyexpr(ex);
			// 	if      (t == "int") ;
			// 	else if (t == "string" || t == "string$")  ex.at(0).tok = "string_expr";
			// 	else    error2("p_print");
			// }
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
		if    (expect("endl"))  emit({ "i", "0" }, "default return command");
		else  p_intexpr();
		require("endl"), nextline();
		emit({ "ret" });
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
		emit({ "jump", while_labels.at(flag_while - break_level) + "end" });
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
		emit({ "jump", while_labels.at(flag_while - cont_level) + "start" });
	}

	void p_if() {
		require("'if");
		int cond = 0;
		string label = "$ctrl_if_" + to_string(++flag_ctrlpoint) + "_";
		emitl(label + "start");
		dsym();
		// first comparison
		p_intexpr(), require("endl"), nextline();
		emit({ "jumpifn", label + to_string(cond+1) }, "first condition");
		p_block();
		emit({ "jump", label + "end" });
		// else-if statements
		while (expect("'else 'if")) {
			emitl(label + to_string(++cond));
			dsym();
			p_intexpr(), require("endl"), nextline();
			emit({ "jumpifn", label+to_string(cond+1) }, "condition "+to_string(cond));
			p_block();
			// place.push_back( emit_placeholder() );
			emit({ "jump", label + "end" });
		}
		// if all conditions fail, we end up here
		emitl(label + to_string(++cond));
		// last else, guaranteed execution
		if (expect("'else endl")) {
			dsym();
			nextline();
			p_block();
		}
		// block end
		require("'end 'if endl"), nextline();
		emitl(label + "end");
	}

	void p_while() {
		require("'while");
		flag_while++;
		string label = "$ctrl_while_" + to_string(++flag_ctrlpoint) + "_";
		while_labels.push_back(label);
		emitl(label + "start");
		dsym();
		// comparison
		p_intexpr(), require("endl"), nextline();
		emit({ "jumpifn", label + "end" });
		p_block();
		// block end
		require("'end 'while endl"), nextline();
		emit({ "jump", label + "start" });  // loop
		emitl(label + "end");
		while_labels.pop_back();
		flag_while--;
	}



	// --- Expressions --- 

	void   p_intexpr() { p_expr_comp() == "int" || error(ERR_EXPECTED_INT); }
	// void   p_strexpr(Node& p) { auto t = p_expr_or(p);  t == "string" || t == "string$" || error2("p_strexpr"); }
	// string p_anyexpr(Node& p) { return p_expr_or(p); }

	// string p_expr_or(Node& p) {
	// 	auto t = p_expr_and(p);
	// 	if (expect("'| '|")) {
	// 		auto lhs = p.pop();
	// 		auto& l  = p.pushlist();
	// 			l.pushtoken("or");
	// 			l.push(lhs);
	// 		auto u = p_expr_or(l);  // parse rhs
	// 		if (t != "int" || t != u)  error();
	// 	}
	// 	return t;
	// }

	// string p_expr_and(Node& p) {
	// 	auto t = p_expr_comp(p);
	// 	if (expect("'& '&")) {
	// 		auto lhs = p.pop();
	// 		auto& l  = p.pushlist();
	// 			l.pushtoken("and");
	// 			l.push(lhs);
	// 		auto u = p_expr_and(l);  // parse rhs
	// 		if (t != "int" || t != u)  error();
	// 	}
	// 	return t;
	// }

	string p_expr_comp() {
		auto t = p_expr_add();
		if (expect("@'= @'=") || expect("@'! @'=") || expect("@'< @'=") || expect("@'> @'=") || expect("@'<") || expect("@'>")) {
			string op = presults.at(0) + (presults.size() > 1 ? presults.at(1) : "");

			// if (t == "string")  t = "string$";  // normalise string
			// if      (t == "int")  l.pushtoken("comp"+op);
			// else if (t == "string$" && op == "==")  l.pushtoken("strcmp");
			// else if (t == "string$" && op == "!=")  l.pushtoken("strncmp");
			// else    error();
			// l.push(lhs);  // reappend lhs
			// auto u = p_expr_add(l);  // parse rhs
			// if (u == "string")  u = "string$";  // normalise string
			// if (t != u)  error();
			// return "int";

			string opcode;
			if (op == "==")  opcode = "eq";
			if (op == "!=")  opcode = "neq";
			if (op == "<" )  opcode = "lt";
			if (op == ">" )  opcode = "gt";
			if (op == "<=")  opcode = "lte";
			if (op == ">=")  opcode = "gte";
			
			auto u = p_expr_add();
			if (t != "int" || t != u)  error(ERR_EXPECTED_INT);
			emit({ opcode });
		}
		return t;
	}
	
	string p_expr_add() {
		auto t = p_expr_mul();
		while (expect("@'+") || expect("@'-")) {
			// auto sign = presults.at(0);
			// if (t == "string")  t = "string$";  // normalise string
			// if      (t == "int")  l.pushtoken(presults.at(0) == "+" ? "add" : "sub");
			// else if (t == "string$" && presults.at(0) == "+")  l.pushtoken("strcat");
			// else    error();
			// l.push(lhs);  // reappend lhs
			// auto u = p_expr_mul(l);  // parse rhs
			// if (u == "string")  u = "string$";  // normalise string
			// if (t != u)  error();
			// if (t == "int")  emit({ (sign == "+" ? "add.v" : "sub.v"), "$top", "$pop" });
			
			string opcode = presults.at(0) == "+" ? "add" : "sub";
			auto u = p_expr_mul();
			if (t != "int" || t != u)  error(ERR_EXPECTED_INT);  // ERR_UNMATCHED_TYPES
			emit({ opcode });
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
		if      (expect("'true"))         return emit({ "i", "1" }, "true" ),  "int";
		else if (expect("'false"))        return emit({ "i", "0" }, "false"), "int";
		else if (expect("@integer"))      return emit({ "i", presults.at(0) }), "int";
		else if (peek("identifier '("))   return p_expr_call();
		else if (peek("identifier"))      return p_varpath();
		// else if (expect("@literal"))      return p.pushliteral(presults.at(0)),  "string$";
		else if (eol())                   error(ERR_UNEXPECTED_EOL);
		return error(ERR_UNKNOWN_ATOM), "nil";
	}

	string p_varpath() {
		string type = p_varpath_base();
		// while (!eof())
		// 	if       (peek("'."))  lhs = p.pop(),  type = p_varpath_prop(type, p),    p.back().push(lhs);
		// 	else if  (peek("'["))  lhs = p.pop(),  type = p_varpath_arrpos(type, p),  p.back().push(lhs);
		// 	else     break;
		return type;
	}
	
	string p_varpath_base() {
		require("@identifier");
		auto name = presults.at(0);
		// local vars
		if (cfuncname.length() && functions.at(cfuncname).args.count(name)) {
			auto& d = functions.at(cfuncname).args.at(name);
			emit({ "get", name });
			return d.type;
		}
		else if (cfuncname.length() && functions.at(cfuncname).locals.count(name)) {
			auto& d = functions.at(cfuncname).locals.at(name);
			emit({ "get", name });
			return d.type;
		}
		// global vars
		else if (globals.count(name)) {
			auto&d = globals[name];
			emit({ "get_global", name });
			return d.type;
		}
		else  return error(ERR_UNDEFINED_VARIABLE), "nil";
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
		return p_expr_call_user();
	}

	string p_expr_call_user() {
		require("@identifier '(");
		auto fname = presults.at(0);
		if (!functions.count(fname))  error(ERR_UNDEFINED_FUNCTION);
		int argc = 0;
		while (!eol() && !peek("')")) {
			p_intexpr();
			argc++;
			if (argat(fname, argc-1).type != "int")  error(ERR_EXPECTED_INT);
			if (!expect("',"))  break;
		}
		require("')");
		if (argc != functions.at(fname).args.size())  error(ERR_ARGUMENT_COUNT_MISMATCH);
		emit({ "call", fname });
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
};