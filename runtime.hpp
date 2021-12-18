#pragma once
#include "helpers.hpp"
#include <iostream>
#include <string>
#include <vector>
#include <map>
using namespace std;


struct Runtime {
	struct StackFrame { map<string, int32_t> vars; };
	struct HeapObject { int32_t index; string type; vector<int32_t> data; };
	Node prog;
	StackFrame globals;
	vector<StackFrame> frames;
	map<int32_t, HeapObject> heap;
	map<string, int32_t> defines;
	int32_t heap_top = 0;
	// runtime flags
	int32_t flag_while = 0, flag_memtrace = 0;

	// --- Init ---
	Runtime(const Node& _prog) : prog(_prog) { }



	// --- Program blocks ---

	void r_prog() {
		// initialize in order
		for (auto& t : findsection("type_defs").list)
			if (t.cmd() == "type")  r_type(t);
		for (auto& d : findsection("dim_global").list)
			if (d.cmd() == "dim")  globals.vars[d.tokat(1)] = 0;
		r_block_special(findsection("setup"));
		r_func("main", {});
		r_block_special(findsection("teardown"));
		printf("::end::  def %d, global %d, heap %d \n", defines.size(), globals.vars.size(), heap.size() );
	}

	void r_type(const Node& type) {
		string tname = type.tokat(1);
		int32_t idx = 0;
		for (auto& p : type.list)
			if (p.cmd() == "dim")
				defines[ tname+"::"+p.tokat(1) ] = idx++;
		defines[ tname+"::$len" ] = idx;
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
			if      (n.tok == "setup")          ;  // ignore this
			else if (n.tok == "teardown")       ;  // ignore
			else if (n.cmd() == "malloc")       r_malloc    ( n.tokat(1), n.tokat(2), n.tokat(3) );
			else if (n.cmd() == "arrmalloc")    r_arrmalloc ( n.tokat(1), n.tokat(2), n.tokat(3), n.at(4).i );
			else if (n.cmd() == "free")         r_free      ( n.tokat(1), n.tokat(2) );
			// else if (n.cmd() == "arrfree")      r_arrfree   ( n.tokat(1), n.tokat(2) );
			else if (n.cmd() == "set_global")   r_set(n);
			else if (n.cmd() == "set_local")    r_set(n);
			else if (n.cmd() == "strcpy")       r_strcpy(n);
			else    error2("special block error: "+n.cmd());
	}

	void r_block(const Node& blk) {
		cout.flush();  // TODO: temp loop yield indicator
		for (auto& n : blk.list)
			if      (n.tok == "block")            ;  // ignore this
			else if (n.cmd() == "print")          r_print(n);
			else if (n.cmd() == "input")          r_input(n);
			else if (n.cmd() == "if")             r_if(n);
			else if (n.cmd() == "while")          r_while(n);
			else if (n.cmd() == "call")           r_call(n);
			else if (n.cmd() == "return")         r_return(n);
			else if (n.cmd() == "break")          r_break(n);
			else if (n.cmd() == "continue")       r_continue(n);
			else if (n.cmd() == "set_global")     r_set(n);
			else if (n.cmd() == "set_local")      r_set(n);
			else if (n.cmd() == "set_property")   r_set(n);
			else if (n.cmd() == "set_arraypos")   r_set(n);
			else if (n.cmd() == "strcpy")         r_strcpy(n);
			// else if (n.cmd() == "memcpy")         r_memcpy(n);
			else if (n.cmd() == "redim")          r_redim(n);
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

	void r_input(const Node& p) {
		int32_t ptr = r_expr(p.at(1));
		string  inp, prompt = r_strexpr(p.at(2));
		printf("%s", prompt.c_str());
		getline(cin, inp);
		string_to_ptr(inp, ptr);
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
		if      (n.cmd() == "set_global")     globals.vars.at(n.tokat(1)) = r_expr(n.at(3));
		else if (n.cmd() == "set_local")      frames.back().vars.at(n.tokat(1))  = r_expr(n.at(3));
		else if (n.cmd() == "set_property")   heapat( r_expr(n.at(3)), n.tokat(1) ) = r_expr(n.at(4));
		else if (n.cmd() == "set_arraypos")   heapat( r_expr(n.at(3)), r_expr(n.at(1)) ) = r_expr(n.at(4));
		else    error2("set error");
	}
	
	void r_strcpy(const Node& n) {
		int32_t ptr = r_expr(n.at(1));
		string s    = r_strexpr(n.at(2));
		// auto& d     = heapdesc(ptr);
		// d.data.resize(s.length());
		// for (int i = 0; i < s.length(); i++)
		// 	d.data[i] = s[i];
		string_to_ptr(s, ptr);
	}

	void r_redim(const Node& n) {
		int32_t ptr = r_expr(n.at(1));
		int32_t len = r_expr(n.at(2));
		mem_resize(ptr, len);
	}



	// --- Expressions ---

	string r_strexpr(const Node& n) {
		if      (n.type == NT_STRLITERAL)     return n.tok;
		else if (n.cmd() == "strcat")         return r_strexpr(n.at(1)) + r_strexpr(n.at(2));
		else if (n.cmd() == "get_global")     return ptr_to_string( r_expr(n) );
		else if (n.cmd() == "get_local")      return ptr_to_string( r_expr(n) );
		else if (n.cmd() == "get_property")   return ptr_to_string( r_expr(n) );
		else if (n.cmd() == "get_arraypos")   return ptr_to_string( r_expr(n) );
		
		printf(">> strexpr error\n"), n.show();
		return error2("strexpr error"), "nil";
	}

	int32_t r_expr(const Node& n) {
		if      (n.tok == "true")             return 1;
		else if (n.tok == "false")            return 0;
		else if (n.type == NT_INTEGER)        return n.i;
		else if (n.cmd() == "or")             return r_expr(n.at(1)) || r_expr(n.at(2));
		else if (n.cmd() == "and")            return r_expr(n.at(1)) && r_expr(n.at(2));
		else if (n.cmd() == "comp==")         return r_expr(n.at(1)) == r_expr(n.at(2));
		else if (n.cmd() == "comp!=")         return r_expr(n.at(1)) != r_expr(n.at(2));
		else if (n.cmd() == "comp<")          return r_expr(n.at(1)) <  r_expr(n.at(2));
		else if (n.cmd() == "comp>")          return r_expr(n.at(1)) >  r_expr(n.at(2));
		else if (n.cmd() == "comp<=")         return r_expr(n.at(1)) <= r_expr(n.at(2));
		else if (n.cmd() == "comp>=")         return r_expr(n.at(1)) >= r_expr(n.at(2));
		else if (n.cmd() == "add")            return r_expr(n.at(1)) +  r_expr(n.at(2));
		else if (n.cmd() == "sub")            return r_expr(n.at(1)) -  r_expr(n.at(2));
		else if (n.cmd() == "mul")            return r_expr(n.at(1)) *  r_expr(n.at(2));
		else if (n.cmd() == "div")            return r_expr(n.at(1)) /  r_expr(n.at(2));
		else if (n.cmd() == "get_global")     return globals.vars.at(n.tokat(1));
		else if (n.cmd() == "get_local")      return frames.back().vars.at(n.tokat(1));
		else if (n.cmd() == "get_property")   return heapat( r_expr(n.at(3)), n.tokat(1) );
		else if (n.cmd() == "get_arraypos")   return heapat( r_expr(n.at(3)), r_expr(n.at(1)) );
		else if (n.cmd() == "strcmp")         return r_strexpr(n.at(1)) == r_strexpr(n.at(2));
		else if (n.cmd() == "strncmp")        return r_strexpr(n.at(1)) != r_strexpr(n.at(2));
		else if (n.cmd() == "strlen")         return heapdesc( r_expr(n.at(1)) ).data.size();
		else if (n.cmd() == "charat")         return r_charat( r_strexpr(n.at(1)), r_expr(n.at(2)) );
		else if (n.cmd() == "sizeof")         return heapdesc( r_expr(n.at(1)) ).data.size();
		else if (n.cmd() == "substr")         return r_substr( r_expr(n.at(1)), r_expr(n.at(2)), r_expr(n.at(3)), r_expr(n.at(4)) );
		else if (n.cmd() == "call")           return r_call(n);

		printf(">> expr error\n"), n.show();
		return error2("expr error");
	}



	// --- Special runtime functions ---

	void r_malloc(const string& locality, const string& name, const string& type) {
		auto ptr = mem_malloc(type);
		if      (locality == "global")  globals.vars.at(name) = ptr;
		else if (locality == "local")   frames.back().vars.at(name) = ptr;
		else    error2("malloc error");
	}
	void r_arrmalloc(const string& locality, const string& name, const string& type, int32_t length) {
		auto ptr = mem_arrmalloc(type, length);
		if      (locality == "global")  globals.vars.at(name) = ptr;
		else if (locality == "local")   frames.back().vars.at(name) = ptr;
		else    error2("arrmalloc error");
	}
	void r_free(const string& locality, const string& name) {
		int32_t ptr = 0;
		if      (locality == "global")  ptr = globals.vars.at(name);
		else if (locality == "local")   ptr = frames.back().vars.at(name);
		else    error2("free error");
		mem_free(ptr);
	}
	// void r_arrfree(const string& locality, const string& name) {
	// 	int32_t ptr = 0;
	// 	if      (locality == "global")  ptr = globals.vars.at(name);
	// 	else if (locality == "local")   ptr = frames.back().vars.at(name);
	// 	else    error2("arrfree error");
	// 	mem_free(ptr);
	// }
	int32_t r_charat(const string& str, int32_t pos) {
		return pos < 0 || pos >= str.length() ? 0 : str[pos];
	}
	int32_t r_substr(int32_t dest, int32_t src, int32_t pos, int32_t len) {
		auto& d = heapdesc(dest);
		auto& s = heapdesc(src);
		d.data = vector<int32_t>( s.data.begin()+pos, s.data.begin()+pos+len );
		return d.data.size();
	}
	string ptr_to_string(int32_t ptr) {
		string s;
		for (auto c : heapdesc(ptr).data)
			s += char(c);
		return s;
	}
	void string_to_ptr(const string& s, int32_t ptr) {
		auto& d = heapdesc(ptr);
		d.data.resize(s.length());
		for (int i = 0; i < s.length(); i++)
			d.data[i] = s[i];
	}



	// --- Internal memory management ---

	HeapObject& heapdesc(int32_t ptr) {
		try                    { return heap.at(ptr); }
		catch (out_of_range e) { error2("memory fault at: "+ to_string(ptr));  throw DBRunError(); }
	}
	int32_t& heapat(int32_t ptr, int32_t offset) {
		try                    { return heap.at(ptr).data.at(offset); }
		catch (out_of_range e) { error2("memory fault at: "+ to_string(ptr));  throw DBRunError(); }
	}
	int32_t& heapat(int32_t ptr, const string& prop) {
		return heapat( ptr, defines.at(prop) );
	}

	int32_t mem_malloc(const string& type) {
		int32_t ptr = ++heap_top;
		heap[ptr] = { ptr, type };
		if (flag_memtrace)
			printf("malloc:      %03d   %s \n", ptr, type.c_str() );
		int32_t data = 0;
		// recursively allocate members 
		if (type == "string") ;  // no inner members
		else
			for (auto& d : findtype(type).list)
				if (d.cmd() == "dim") {
					if    (d.tokat(2) == "int")  data = 0;
					else  data = mem_malloc( d.tokat(2) );
					heap[ptr].data.push_back( data );
				}
		// return ptr address
		return ptr;
	}

	int32_t mem_arrmalloc(const string& type, int32_t len) {
		int32_t ptr = ++heap_top;
		// string arrtype = type+"[]";
		heap[ptr] = { ptr, type };
		if (flag_memtrace)
			printf("arrmalloc:   %03d   %s \n", ptr, type.c_str() );
		// allocate each member
		heap[ptr].data.resize(len);
		if (basetype(type) != "int")
			for (int32_t i = 0; i < len; i++)
				heap[ptr].data[i] = mem_malloc(basetype(type));
		return ptr;
	}

	void mem_free(int32_t ptr) {
		auto desc = heapdesc(ptr);
		int32_t offset = 0;
		// recursively deallocate members

		if (is_arraytype(desc.type) && basetype(desc.type) == "int") ;  // free not needed
		else if (is_arraytype(desc.type))
			for (int32_t ptr : desc.data)
				mem_free( ptr );
		else if (desc.type == "string") ;  // no inner members
		
		// if (desc.type == "string") ;  // no inner members
		else
			for (auto& d : findtype(desc.type).list)
				if (d.cmd() == "dim") {
					if    (d.tokat(2) == "int") ;
					else  mem_free( heapat(ptr, offset) );
					offset++;
				}
		// deallocate this
		if (flag_memtrace)
			printf("free:        %03d   %s \n", ptr, heapdesc(ptr).type.c_str() );
		heap.erase(ptr);
	}

	// void mem_arrfree(int32_t ptr) {
	// 	auto desc = heapdesc(ptr);
	// 	if (!desc.isarray)  error2("mem_arrfree");
	// 	int32_t offset = 0;
	// 	// deallocate each member
	// 	if (desc.type == "int") ;
	// 	else
	// 		for (int32_t ptr : desc.data)
	// 			mem_free( ptr );
	// 	// deallocate this
	// 	printf("arrfree:     %03d   %s[] \n", ptr, heapdesc(ptr).type.c_str() );
	// 	heap.erase(ptr);
	// }

	void mem_resize(int32_t ptr, int32_t newlen) {
		int32_t oldlen = heap.at(ptr).data.size();
		auto type      = heap.at(ptr).type;
		auto btype     = basetype(type);
		// if newlen is shorter, free memory
		if (btype != "int")
			for (int32_t i = oldlen-1; i >= newlen; i--)
				mem_free( heap.at(ptr).data[i] );
		// resize
		heap.at(ptr).data.resize(newlen, 0);
		if (flag_memtrace)
			printf("resize:      %03d   %d -> %d \n", ptr, oldlen, newlen );
		// if newlen is longer, construct new memory
		if (btype != "int")
			for (int32_t i = oldlen; i < newlen; i++)
				heap.at(ptr).data[i] = mem_malloc(btype);
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

}; // end Runtime