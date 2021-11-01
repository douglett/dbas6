#pragma once
#include "helpers.hpp"
using namespace std;


struct Runtime {
	struct StackFrame { map<string, int32_t> vars; };
	Node prog;
	vector<StackFrame> frames;

	Runtime(const Node& _prog) : prog(_prog) {
		frames = { {} };
	}


	void r_prog() {
		for (const auto& n : prog.list)
			if      (n.cmd() == "section" && n.tokat(1) == "type") ;
			else if (n.cmd() == "section" && n.tokat(1) == "dimglobal")
				for (auto& d : n.list)
					if (d.cmd() == "dim")  frames.back().vars[d.tokat(1)] = 0;
		// run main function
		r_func("main", {});
	}

	const Node& findfunc(const string& fname) const {
		for (auto& n : prog.list)
			if (n.cmd() == "section" && n.tokat(1) == "function")
				for (auto& f : n.list)
					if (f.cmd() == "function" && f.tokat(1) == fname)
						return f;
		error2("missing function: "+fname);
		throw DBRunError();
	}

	void r_func(const string& fname, vector<int32_t> args) {
		auto& func = findfunc(fname);
		frames.push_back({ });
		for (auto& n : func.list)
			if (n.cmd() == "args") {
				for (auto& d : n.list)
					if (d.cmd() == "dim")  frames.back().vars[d.tokat(1)] = 0;
			}
			else if (n.cmd() == "dimlocal") {
				for (auto& d : n.list)
					if (d.cmd() == "dim")  frames.back().vars[d.tokat(1)] = 0;
			}
			else if (n.cmd() == "block") {
				r_block(n);
			}
		frames.pop_back();
	}

	void r_block(const Node& blk) {
		// printf("block\n");
		for (auto& n : blk.list) {
			if      (n.cmd() == "print")       r_print(n);
			else if (n.cmd() == "set_global")  r_let(n);
			else if (n.cmd() == "set_local")   r_let(n);
			else if (n.cmd() == "if")          r_if(n);
			else if (n.tok == "block")         ;  // ignore this
			else    error2("block error: "+n.cmd());
		}
	}

	void r_print(const Node& p) {
		for (auto& n : p.list)
			if      (n.tok == "print") ;
			else if (n.type == NT_STRLITERAL)  printf("%s", n.tok.c_str());
		printf("\n");
	}
	
	void r_let(const Node& n) {
		if      (n.cmd() == "set_global")  frames.front().vars.at(n.tokat(1)) = r_expr(n.at(2));
		else if (n.cmd() == "set_local")   frames.back().vars.at(n.tokat(1))  = r_expr(n.at(2));
		else    error2("let error");
	}

	void r_if(const Node& n) {
		for (int i = 1; i < n.list.size(); i += 2) {
			auto& cond = n.list.at(i);
			auto& blk  = n.list.at(i+1);
			if (r_expr(cond)) {
				r_block(blk);
				break;
			}
		}
	}

	int r_expr(const Node& n) {
		if      (n.tok == "true")          return 1;
		else if (n.tok == "false")         return 0;
		else if (is_integer(n.tok))        return stoi(n.tok);
		else if (n.cmd() == "get_global")  return frames.front().vars.at(n.tokat(1));
		else if (n.cmd() == "comp==")      return r_expr(n.at(1)) == r_expr(n.at(2));
		else if (n.cmd() == "add")         return r_expr(n.at(1)) +  r_expr(n.at(2));
		else if (n.cmd() == "sub")         return r_expr(n.at(1)) -  r_expr(n.at(2));

		printf(">> expr error\n");
		n.show();
		return error2("expr error");
	}



	// helpers
	int error() const {
		throw DBRunError();
	}
	int error2(const string& msg) const {
		// Temporary WIP parser errors
		DBRunError d;
		d.error_string += " :: " + msg;
		throw d;
	}
};