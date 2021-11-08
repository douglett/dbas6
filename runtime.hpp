#pragma once
#include "helpers.hpp"
using namespace std;


struct Runtime {
	struct StackFrame { map<string, int32_t> vars; };
	struct HeapObject { int32_t index; string type; vector<int32_t> data; };
	Node prog;
	vector<StackFrame> frames;
	map<int32_t, HeapObject> heap;
	int32_t heap_top = 0;

	Runtime(const Node& _prog) : prog(_prog) {
		frames = { {} };
	}


	// find functions
	const Node& findsection(const string& section) const {
		for (const auto& n : prog.list)
			if (n.cmd() == section)  return n;
		error2("missing section: "+section);
		throw DBRunError();
	}
	const Node& findfunc(const string& fname) const {
		for (auto& f : findsection("function_defs").list)
			if (f.cmd() == "function" && f.tokat(1) == fname)
				return f;
		error2("missing function: "+fname);
		throw DBRunError();
	}


	// special runtime functions
	void r_malloc(const string& locality, const string& name, const string& type) {
		heap[++heap_top] = { heap_top, type };
		if      (locality == "global")  frames.front().vars.at(name) = heap_top;
		else if (locality == "local")   frames.back().vars.at(name) = heap_top;
		else    error2("malloc error");
		printf("malloc:  %d \n", heap_top);
	}
	void r_free(const string& locality, const string& name) {
		int32_t addr = 0;
		if      (locality == "global")  addr = frames.front().vars.at(name);
		else if (locality == "local")   addr = frames.back().vars.at(name);
		if (addr <= 0 || !heap.count(addr))  error2("free error");
		heap.erase(addr);
		printf("free:  %d \n", addr);
	}
	string ptr_to_string(int32_t ptr) {
		string s;
		for (auto c : heap.at(ptr).data)
			s += char(c);
		return s;
	}


	// program blocks
	void r_prog() {
		// initialize in order
		findsection("type_defs");
		for (auto& d : findsection("dim_global").list)
			if (d.cmd() == "dim")  frames.back().vars[d.tokat(1)] = 0;
		r_block_special(findsection("setup"));
		r_func("main", {});
		r_block_special(findsection("teardown"));
	}

	int32_t r_func(const string& fname, vector<int32_t> args) {
		auto& func = findfunc(fname);
		frames.push_back({ });
			frames.back().vars["$ret"] = 0;  // default return value
		for (auto& n : func.list)
			// build arguments
			if (n.cmd() == "args") {
				int argc = 0;
				for (auto& d : n.list)
					if (d.cmd() == "dim")  frames.back().vars[d.tokat(1)] = args.at(argc++);
			}
			// build local variables
			else if (n.cmd() == "dim_local") {
				for (auto& d : n.list)
					if (d.cmd() == "dim")  frames.back().vars[d.tokat(1)] = 0;
			}
			// run block
			else if (n.cmd() == "block") {
				try { r_block(n); }
				catch (DBCtrlReturn r) { }
			}
			// cleanup
			// ...
		int32_t _ret = frames.back().vars["$ret"];
		frames.pop_back();
		return _ret;
	}

	int32_t r_call(const Node& n) {
		auto fname = n.tokat(1);
		vector<int32_t> arglist;
		for (auto& arg : n.at(2).list)
			arglist.push_back(r_expr(arg));
		return r_func(fname, arglist);
	}

	void r_block_special(const Node& blk) {
		for (auto& n : blk.list)
			if      (n.tok == "setup")         ;  // ignore this
			else if (n.tok == "teardown")      ;  // ignore
			else if (n.cmd() == "malloc")      r_malloc(n.tokat(1), n.tokat(2), n.tokat(3));
			else if (n.cmd() == "free")        r_free(n.tokat(1), n.tokat(2));
			else    error2("special block error: "+n.cmd());
	}

	void r_block(const Node& blk) {
		// printf("block\n");
		for (auto& n : blk.list)
			if      (n.tok == "block")         ;  // ignore this
			else if (n.cmd() == "print")       r_print(n);
			else if (n.cmd() == "if")          r_if(n);
			else if (n.cmd() == "call")        r_call(n);
			else if (n.cmd() == "return")      r_return(n);
			else if (n.cmd() == "set_global")  r_set(n);
			else if (n.cmd() == "set_local")   r_set(n);
			else if (n.cmd() == "strcpy")      r_strcpy(n);
			else    error2("block error: "+n.cmd());
	}

	void r_print(const Node& p) {
		for (auto& n : p.list)
			if      (n.tok == "print") ;
			else if (n.type == NT_STRLITERAL)   printf("%s", n.tok.c_str() );
			else if (n.cmd() == "int_expr")     printf("%d", r_expr(n.at(1)) );
			else if (n.cmd() == "string_expr")  printf("%s", r_strexpr(n.at(1)).c_str() );
			else    error2("print error");
		printf("\n");
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
	
	void r_return(const Node& n) {
		frames.back().vars.at("$ret") = r_expr(n.at(1));
		throw DBCtrlReturn();
	}

	void r_set(const Node& n) {
		// format: cmd, varpath, vpath_type, expr
		if      (n.cmd() == "set_global")  frames.front().vars.at(n.tokat(1)) = r_expr(n.at(3));
		else if (n.cmd() == "set_local")   frames.back().vars.at(n.tokat(1))  = r_expr(n.at(3));
		else    error2("let error");
	}
	
	void r_strcpy(const Node& n) {
		int32_t ptr = r_expr(n.at(1));
		string s    = r_strexpr(n.at(2));
		auto& data  = heap.at(ptr).data;
		data.resize(s.length());
		for (int i = 0; i < s.length(); i++)
			data[i] = s[i];
	}

	string r_strexpr(const Node& n) {
		if      (n.type == NT_STRLITERAL)  return n.tok;
		else if (n.cmd() == "strcat")      return r_strexpr(n.at(1)) + r_strexpr(n.at(2));
		else if (n.cmd() == "get_global")  return ptr_to_string( r_expr(n) );
		else if (n.cmd() == "get_local")   return ptr_to_string( r_expr(n) );
		
		printf(">> strexpr error\n"), n.show();
		return error2("strexpr error"), "nil";
	}

	int32_t r_expr(const Node& n) {
		if      (n.tok == "true")          return 1;
		else if (n.tok == "false")         return 0;
		else if (is_integer(n.tok))        return stoi(n.tok);
		else if (n.cmd() == "comp==")      return r_expr(n.at(1)) == r_expr(n.at(2));
		else if (n.cmd() == "add")         return r_expr(n.at(1)) +  r_expr(n.at(2));
		else if (n.cmd() == "sub")         return r_expr(n.at(1)) -  r_expr(n.at(2));
		else if (n.cmd() == "mul")         return r_expr(n.at(1)) *  r_expr(n.at(2));
		else if (n.cmd() == "div")         return r_expr(n.at(1)) /  r_expr(n.at(2));
		else if (n.cmd() == "get_global")  return frames.front().vars.at(n.tokat(1));
		else if (n.cmd() == "get_local")   return frames.back().vars.at(n.tokat(1));
		else if (n.cmd() == "call")        return r_call(n);

		printf(">> expr error\n"), n.show();
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