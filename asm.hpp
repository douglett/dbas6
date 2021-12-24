#pragma once
#include <string>
#include <vector>
#include <map>
#include <fstream>
using namespace std;


struct DBRunError : std::exception {
	string error_string;
	DBRunError(const string& msg, int pc) {
		error_string = msg + "  (line " + to_string(pc+1) + ")";
	}
	virtual const char* what() const noexcept {
		return error_string.c_str();
	}
};


struct ASM {
	static const int32_t STACK_SIZE = 1024;
	typedef  map<string, int32_t>  StackFrame;

	int32_t pc = 0, heap_top = 0, stack_top = 0;
	StackFrame                     globals;
	vector<StackFrame>             fstack;
	// vector<int32_t>                stack = vector<int32_t>(STACK_SIZE, 0);
	int32_t                        stack[STACK_SIZE] = {0};
	map<int32_t, vector<int32_t>>  heap;
	vector<vector<string>>         prog;


	// --- parsing ---
	char escapechar(char c) {
		switch (c) {
		case 'n':  return '\n';
		case '"':  return '"';
		}
		throw DBRunError(string("unknown escape: [")+c+"]", pc);
	}
	vector<string> splitln(const string& str) {
		vector<string> vs;
		string s;
		int instr = 0;
		for (auto c : str)
			if      (instr == 1)  { if (c == '\\') instr = 2;  else { s += c;  if (c == '"') instr = 0; } }
			else if (instr == 2)  { s += escapechar(c);  instr = 1; }
			else if (isspace(c))  { if (s.length()) vs.push_back(s), s = ""; }
			else if (c == '"')    { if (s.length()) vs.push_back(s), s = "";  s += c;  instr = 1; }
			else                  { s += c; }
		if (s.length())  vs.push_back(s);
		return vs;
	}
	int load(const string& fname) {
		prog = {};
		fstream fs(fname, ios::in);
		if (!fs.is_open())
			return fprintf(stderr, "error loading file: %s\n", fname.c_str()), 1;
		string s;
		while (getline(fs, s))
			prog.push_back( splitln(s) );
		return 0;
	}


	// --- string handling ---
	string strescape(const string& s) {
		if (s.size() >= 2 && s.front() == '"' && s.back() == '"')
			return s.substr(1, s.length()-2);
		return s;
	}
	vector<int32_t> strtoarr(const string& s) {
		vector<int32_t> v(s.length(), 0);
		for (int i = 0; i < s.length(); i++)
			v[i] = s[i];
		return v;
	}
	string arrtostr(const vector<int32_t>& v) {
		string s(v.size(), 0);
		for (int i = 0; i < v.size(); i++)
			s[i] = char(v[i]);
		return s;

	}
	string arrtostr(int32_t src) {
		return arrtostr( desc(src) );
	}


	// --- control ---
	int32_t findlabel(string label) {
		label += ":";
		for (int32_t i = 0; i < prog.size(); i++) {
			const auto& cmd = prog.at(i);
			if (cmd.size() && cmd[0] == label)
				return i;
		}
		throw DBRunError("missing label", pc);
	}
	void call(string label) {
		fstack.push_back({
			{ "$ret", pc }
		});
		pc = findlabel(label);
	}
	void ret() {
		if (fstack.size() == 0)  throw DBRunError("framestack_underflow", pc);
		pc = fstack.back().at("$ret");
		fstack.pop_back();
	}


	// --- memory access ---
	int32_t& var(const string& id) {
		try                    { return frame().at(id); }
		catch (out_of_range e) { throw DBRunError("local_var_undefined", pc); }
	}
	int32_t& var_global(const string& id) {
		try                    { return globals.at(id); }
		catch (out_of_range e) { throw DBRunError("global_var_undefined", pc); }
	}
	StackFrame frame() {
		try                    { return fstack.at(fstack.size() - 1); }
		catch (out_of_range e) { throw DBRunError("framestack_underflow", pc); }
	}
	int32_t& mem(int32_t src, int32_t offset) {
		try                    { return desc(src).at(offset); }
		catch (out_of_range e) { throw DBRunError("segfault", pc); }
	}
	vector<int32_t>& desc(int32_t src) {
		try                    { return heap.at(src); }
		catch (out_of_range e) { throw DBRunError("segfault", pc); }	
	}
	void push(int32_t i) {
		if (stack_top >= STACK_SIZE-1)  throw DBRunError("stack_overflow", pc);
		stack[++stack_top] = i;
	}
	int32_t pop() {
		if (stack_top <= 0)  throw DBRunError("stack_underflow", pc);
		return stack[stack_top--];
	}
	int32_t& peek() {
		if (stack_top < 0 || stack_top >= STACK_SIZE)  throw DBRunError("stack_bad_state", pc);
		return stack[stack_top];
	}


	// --- heap memory ---
	// int32_t r_malloc(int32_t size) {
	// 	heap[++heap_top] = vector<int32_t>(size, 0);
	// 	return heap_top;
	// }
	// void r_free(int32_t src) {
	// 	heap.erase(src);
	// }
	// void r_memcopy(int32_t dest, int32_t src) {
	// 	desc(dest) = desc(src);
	// }
	// void r_memcopy(int32_t dest, const string& src) {
	// 	desc(dest) = strtoarr(src);
	// }
	// void r_concat(int32_t dest, int32_t src) {
	// 	desc(dest).insert( desc(dest).end(), desc(src).begin(), desc(src).end() );
	// }
	// void r_concat(int32_t dest, const string& src) {
	// 	desc(dest).insert( desc(dest).end(), src.begin(), src.end() );
	// }
	// void r_slice(int32_t dest, int32_t src, int32_t start, int32_t length) {
	// 	desc(dest) = vector<int32_t>( desc(src).begin()+start, desc(src).begin()+start+length );
	// }



	// --- main loop ---
	void mainloop() {
		int32_t t = 0;
		string s;
		pc = 0;

		while (pc < prog.size()) {
			const auto& cmd = prog.at(pc);
			// meta
			if      (cmd.size() == 0)            ;  // noop
			else if (cmd[0].front() == ';')      ;  // comment line
			else if (cmd[0].back()  == ':')      ;  // label
			// variables
			else if (cmd[0] == "let")            frame()[cmd[1]] = 0;
			else if (cmd[0] == "let_global")     globals[cmd[1]] = 0;
			else if (cmd[0] == "get")            push( var(cmd[1]) );
			else if (cmd[0] == "get_global")     push( var_global(cmd[1]) );
			else if (cmd[0] == "set")            var(cmd[1]) = pop();
			else if (cmd[0] == "set_global")     var_global(cmd[1]) = pop();
			else if (cmd[0] == "i")              push( stoi(cmd[2]) );
			// basic
			else if (cmd[0] == "add")            t = pop(),  peek() += t;
			else if (cmd[0] == "sub")            t = pop(),  peek() -= t;
			else if (cmd[0] == "mul")            t = pop(),  peek() *= t;
			else if (cmd[0] == "div")            t = pop(),  peek() /= t;
			else if (cmd[0] == "eq")             t = pop(),  peek() =  peek() == t;
			else if (cmd[0] == "neq")            t = pop(),  peek() =  peek() != t;
			else if (cmd[0] == "lt")             t = pop(),  peek() =  peek() <  t;
			else if (cmd[0] == "gt")             t = pop(),  peek() =  peek() >  t;
			else if (cmd[0] == "lte")            t = pop(),  peek() =  peek() <= t;
			else if (cmd[0] == "gte")            t = pop(),  peek() =  peek() >= t;
			else if (cmd[0] == "drop")           pop();
			// control
			else if (cmd[0] == "jump")           pc = findlabel(cmd[1]);
			else if (cmd[0] == "jumpif")         pc = var(cmd[2]) ? findlabel(cmd[1]) : pc;
			else if (cmd[0] == "jumpifn")        pc = var(cmd[2]) ? pc : findlabel(cmd[1]);
			else if (cmd[0] == "call")           call(cmd[1]);
			else if (cmd[0] == "ret")            ret();
			else if (cmd[0] == "halt")           break;
			// printing
			else if (cmd[0] == "print")          printf("%d", pop() );
			else if (cmd[0] == "print_lit")      printf("%s", strescape(cmd[1]).c_str() );
			// else if (cmd[0] == "print_str")      printf("%s", strescape(cmd[1]).c_str() );
			else if (cmd[0] == "println")        printf("%d\n", pop() );


			// // memory
			// else if (cmd[0] == "malloc.i")    t = r_malloc( stoi(cmd[2]) ),  var(cmd[1]) = t;
			// else if (cmd[0] == "malloc.v")    t = r_malloc( var(cmd[2]) ),   var(cmd[1]) = t;
			// else if (cmd[0] == "free")        r_free( var(cmd[1]) );
			// else if (cmd[0] == "get.i")       var(cmd[1]) =  mem( var(cmd[2]), stoi(cmd[3]) );
			// else if (cmd[0] == "get.v")       var(cmd[1]) =  mem( var(cmd[2]), var(cmd[3]) );
			// else if (cmd[0] == "put.ii")      mem( var(cmd[1]), stoi(cmd[2]) ) =  stoi(cmd[3]);
			// else if (cmd[0] == "put.iv")      mem( var(cmd[1]), stoi(cmd[2]) ) =  var(cmd[3]);
			// else if (cmd[0] == "put.vi")      mem( var(cmd[1]), var(cmd[2])  ) =  stoi(cmd[3]);
			// else if (cmd[0] == "put.vv")      mem( var(cmd[1]), var(cmd[2])  ) =  var(cmd[3]);
			// // arrays
			// else if (cmd[0] == "len")         t = desc( var(cmd[2]) ).size(),  var(cmd[1]) = t;
			// else if (cmd[0] == "copy.v")      t = var(cmd[2]),        r_memcopy( var(cmd[1]), t );
			// else if (cmd[0] == "copy.s")      s = strescape(cmd[2]),  r_memcopy( var(cmd[1]), s );
			// else if (cmd[0] == "cat.v")       t = var(cmd[2]),        r_concat( var(cmd[1]), t );
			// else if (cmd[0] == "cat.s")       s = strescape(cmd[2]),  r_concat( var(cmd[1]), s );
			// else if (cmd[0] == "slice.ii")    r_slice( var(cmd[1]), var(cmd[2]), stoi(cmd[3]), stoi(cmd[4]) );
			// else if (cmd[0] == "slice.iv")    r_slice( var(cmd[1]), var(cmd[2]), stoi(cmd[3]), var(cmd[4]) );
			// else if (cmd[0] == "slice.vi")    r_slice( var(cmd[1]), var(cmd[2]), var(cmd[3]),  stoi(cmd[4]) );
			// else if (cmd[0] == "slice.vv")    r_slice( var(cmd[1]), var(cmd[2]), var(cmd[3]),  var(cmd[4]) );
			// error
			else    throw DBRunError("unknown command: " + cmd[0], pc);
			// next
			pc++;
		}
	}
};