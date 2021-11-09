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
	int32_t flag_while = 0;  // runtime flags

	// --- Init ---
	Runtime(const Node& _prog) : prog(_prog) {
		frames = { {} };
	}



	// --- Find functions ---

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
	const Node& findtype(const string& tname) const {
		for (auto& t : findsection("type_defs").list)
			if (t.cmd() == "type" && t.tokat(1) == tname)
				return t;
		error2("missing type: "+tname);
		throw DBRunError();
	}



	// --- Special runtime functions ---

	void r_malloc_cmd(const string& locality, const string& name, const string& type) {
		auto ptr = r_makeobj(type);		
		if      (locality == "global")  frames.front().vars.at(name) = ptr;
		else if (locality == "local")   frames.back().vars.at(name) = ptr;
		else    error2("malloc error");
	}
	void r_free_cmd(const string& locality, const string& name) {
		int32_t ptr = 0;
		if      (locality == "global")  ptr = frames.front().vars.at(name);
		else if (locality == "local")   ptr = frames.back().vars.at(name);
		else    error2("free error");
		r_freeobj(ptr);
	}
	string ptr_to_string(int32_t ptr) {
		string s;
		for (auto c : heap.at(ptr).data)
			s += char(c);
		return s;
	}



	// --- Internal memory management ---

	int32_t r_makeobj(const string& type) {
		int32_t ptr = ++heap_top;
		heap[ptr] = { ptr, type };
		printf("malloc:   %03d   %s \n", ptr, type.c_str() );
		int32_t data = 0;
		// recursively allocate members 
		if (type == "string") ;  // no inner members
		else
			for (auto& d : findtype(type).list)
				if (d.cmd() == "dim") {
					if    (d.tokat(2) == "int")  data = 0;
					else  data = r_makeobj( d.tokat(2) );
					heap.at(ptr).data.push_back( data );
				}
		// return ptr address
		return ptr;
	}

	int32_t r_freeobj(int32_t ptr) {
		if (ptr <= 0 || !heap.count(ptr))  error2("free fault at: "+ to_string(ptr));
		auto type = heap.at(ptr).type;
		int32_t offset = 0;
		// recursively deallocate members
		if (type == "string") ;  // no inner members
		else
			for (auto& d : findtype(type).list)
				if (d.cmd() == "dim") {
					if   (d.tokat(2) == "int") ;
					else r_freeobj( heap.at(ptr).data.at(offset) );
					offset++;
				}
		// deallocate this
		printf("free:     %03d   %s \n", ptr, type.c_str() );
		return heap.erase(ptr), 0;
	}



	// --- Program blocks ---

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
			// init locals
			else if (n.cmd() == "dim_local") {
				for (auto& d : n.list)
					if (d.cmd() == "dim")  frames.back().vars[d.tokat(1)] = 0;
			}
			// build locals
			else if (n.cmd() == "setup")  r_block_special(n);
			// run block
			else if (n.cmd() == "block") {
				try   { r_block(n); }
				catch (DBCtrlReturn r) { }
			}
			// cleanup
			else if (n.cmd() == "teardown")  r_block_special(n);
		// return
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
			else if (n.cmd() == "malloc")      r_malloc_cmd(n.tokat(1), n.tokat(2), n.tokat(3));
			else if (n.cmd() == "free")        r_free_cmd(n.tokat(1), n.tokat(2));
			else    error2("special block error: "+n.cmd());
	}

	void r_block(const Node& blk) {
		// printf("block\n");
		for (auto& n : blk.list)
			if      (n.tok == "block")         ;  // ignore this
			else if (n.cmd() == "print")       r_print(n);
			else if (n.cmd() == "if")          r_if(n);
			else if (n.cmd() == "while")       r_while(n);
			else if (n.cmd() == "call")        r_call(n);
			else if (n.cmd() == "return")      r_return(n);
			else if (n.cmd() == "break")       r_break(n);
			else if (n.cmd() == "continue")    r_continue(n);
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
	


	// --- General commands ---

	void r_return(const Node& n) {
		frames.back().vars.at("$ret") = r_expr(n.at(1));
		throw DBCtrlReturn();
	}
	void r_break(const Node& n) {
		throw DBCtrlBreak( n.at(1).i );
	}
	void r_continue(const Node& n) {
		throw DBCtrlContinue( n.at(1).i );
	}
	void r_if(const Node& n) {
		for (int i = 1; i < n.list.size(); i += 2)
			if (r_expr( n.at(i) )) {
				r_block( n.at(i+1) );
				break;
			}
	}
	void r_while(const Node& n) {
		flag_while++;
		while (r_expr( n.at(1) ))
			try                      { r_block( n.at(2) ); }
			catch (DBCtrlBreak b)    { if (--b.level > 0)  throw b;  break; }
			catch (DBCtrlContinue c) { if (--c.level > 0)  throw c;  continue; }
		flag_while--;
	}

	void r_set(const Node& n) {
		// format: cmd, varpath, vpath_type, expr
		if      (n.cmd() == "set_global")  frames.front().vars.at(n.tokat(1)) = r_expr(n.at(3));
		else if (n.cmd() == "set_local")   frames.back().vars.at(n.tokat(1))  = r_expr(n.at(3));
		else    error2("set error");
	}
	
	void r_strcpy(const Node& n) {
		int32_t ptr = r_expr(n.at(1));
		string s    = r_strexpr(n.at(2));
		auto& data  = heap.at(ptr).data;
		data.resize(s.length());
		for (int i = 0; i < s.length(); i++)
			data[i] = s[i];
	}



	// --- Expressions ---

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
		else if (n.type == NT_INTEGER)     return n.i;
		else if (n.cmd() == "or")          return r_expr(n.at(1)) || r_expr(n.at(2));
		else if (n.cmd() == "and")         return r_expr(n.at(1)) && r_expr(n.at(2));
		else if (n.cmd() == "comp==")      return r_expr(n.at(1)) == r_expr(n.at(2));
		else if (n.cmd() == "comp!=")      return r_expr(n.at(1)) != r_expr(n.at(2));
		else if (n.cmd() == "comp<")       return r_expr(n.at(1)) <  r_expr(n.at(2));
		else if (n.cmd() == "comp>")       return r_expr(n.at(1)) >  r_expr(n.at(2));
		else if (n.cmd() == "comp<=")      return r_expr(n.at(1)) <= r_expr(n.at(2));
		else if (n.cmd() == "comp>=")      return r_expr(n.at(1)) >= r_expr(n.at(2));
		else if (n.cmd() == "add")         return r_expr(n.at(1)) +  r_expr(n.at(2));
		else if (n.cmd() == "sub")         return r_expr(n.at(1)) -  r_expr(n.at(2));
		else if (n.cmd() == "mul")         return r_expr(n.at(1)) *  r_expr(n.at(2));
		else if (n.cmd() == "div")         return r_expr(n.at(1)) /  r_expr(n.at(2));
		else if (n.cmd() == "get_global")  return frames.front().vars.at(n.tokat(1));
		else if (n.cmd() == "get_local")   return frames.back().vars.at(n.tokat(1));
		else if (n.cmd() == "strcmp")      return r_strexpr(n.at(1)) == r_strexpr(n.at(2));
		else if (n.cmd() == "strncmp")     return r_strexpr(n.at(1)) != r_strexpr(n.at(2));
		else if (n.cmd() == "call")        return r_call(n);

		printf(">> expr error\n"), n.show();
		return error2("expr error");
	}



	// --- Helpers ---
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