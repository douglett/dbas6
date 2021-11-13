// ----------------------------------------
// Various useful functions
// ----------------------------------------
#pragma once
#include <string>
#include <vector>
#include <stdexcept>
using namespace std;



// ----------------------------------------
// Special language exceptions
// ----------------------------------------
struct DBError : std::exception {
	string error_string;
	virtual const char* what() const noexcept {
		return error_string.c_str();
	}
};
struct DBParseError : DBError {
	DBParseError(int lno, const string& ctok) {
		error_string = "DougBasic Syntax error, line " + to_string(lno+1)
			+ ", at token '" + ctok + "'";
	}
};
struct DBRunError : DBError {
	DBRunError() {
		error_string = "DougBasic Runtime error";
	}
};
struct DBCtrl : std::exception { };
struct DBCtrlReturn : DBCtrl { };
struct DBCtrlBreak : DBCtrl {
	int level;
	DBCtrlBreak(int lv) : level(lv) { }
};
struct DBCtrlContinue : DBCtrl {
	int level;
	DBCtrlContinue(int lv) : level(lv) { }
};


// ----------------------------------------
// Useful functions
// ----------------------------------------
int is_identifier(const string& s) {
	if (s.length() == 0)  return 0;
	for (int i = 0; i < s.length(); i++)
		if      (i == 0 && !isalpha(s[i]) && s[i] != '_')  return 0;
		else if (i  > 0 && !isalnum(s[i]) && s[i] != '_')  return 0;
	return 1;
}
int is_integer(const string& s) {
	if (s.length() == 0)  return 0;
	for (int i = 0; i < s.length(); i++)
		if (!isdigit(s[i]))  return 0;
	return 1;
}
int is_strliteral(const string& s) {
	return s.size() >= 2 && s[0] == '"' && s.back() == '"';
}
int is_arraytype(const string& s) {
	return s.length() >= 3 && s[s.length()-2] == '[' && s[s.length()-1] == ']';
}
int is_reftype(const string& s) {
	return s.length() >= 2 && s.back() == '&';
}
int is_arrreftype(const string& s) {
	return s.length() >= 2 && s.back() == '@';
}
string basetype(const string& s) {
	// return is_arraytype(s) ? s.substr(0, s.length()-2) : s;
	if      (is_arraytype(s))   return s.substr(0, s.length()-2);
	else if (is_reftype(s))     return s.substr(0, s.length()-1);
	else if (is_arrreftype(s))  return s.substr(0, s.length()-1);
	else    return s;
}
string clean_strliteral(const string& s) {
	return is_strliteral(s) ? s.substr(1, s.length()-2) : s;
}
vector<string> splitws(const string& str) {
	vector<string> vs;
	stringstream ss(str);
	string s;
	while(ss >> s)  vs.push_back(s);
	return vs;
}
// vector<string> spliton(const string& str, const string& delim) {
// 	vector<string> vs;
// 	int last = 0, next = 0;
// 	while ((next = str.find(delim, last)) != string::npos) {
// 		vs.push_back( str.substr(last, next-last) );
// 		last = next + delim.length();
// 	}
// 	if (last < str.length()-1)  vs.push_back( str.substr(last) );
// 	return vs;
// }
// string join(const vector<string>& vs, const string& delim) {
// 	string str;
// 	int first = 1;
// 	for (auto& s : vs)
// 		if    (first)  str += s,  first = 0;
// 		else  str += delim + s;
// 	return str;
// }



// ----------------------------------------
// Node syntax-tree structure 
// ----------------------------------------
enum NODE_TYPE {
	NT_NIL=0,
	NT_TOKEN,
	NT_LIST,
	NT_STRLITERAL,
	NT_INTEGER,
};

const string NIL_STRING="<nil>";

struct Node {
	NODE_TYPE type = NT_NIL;
	string tok;
	int32_t i = 0;
	vector<Node> list;

	Node() {}
	Node(NODE_TYPE _type) : type(_type) {}
	static Node Token(const string& t) { Node n(NT_TOKEN);  return n.tok = t, n; }
	static Node List() { return Node(NT_LIST); }
	static Node CmdList(const string& t) { Node n(NT_LIST);  return n.pushtoken(t), n; }
	static Node Literal(const string& lit) { Node n(NT_STRLITERAL);  return n.tok = clean_strliteral(lit), n; }
	static Node Integer(int32_t i) { Node n(NT_INTEGER);  return n.i = i, n; }
	static Node Integer(const string& istr) { Node n(NT_INTEGER);  return n.i = stoi(istr), n; }

	Node  pop() {
		assert(type == NT_LIST && list.size() > 0);
		auto n = list.back();
		return list.pop_back(), n;
	}
	Node& push(const Node& n) {
		assert(type == NT_LIST);
		list.push_back( n );
		return list.back();		
	}
	Node& pushlist() { return push( Node::List() ); }
	Node& pushcmdlist(const string& t) { return push( Node::CmdList(t) ); }
	Node& pushliteral(const string& lit) { return push( Node::Literal(lit) ); }
	Node& pushint(int32_t i) { return push( Node::Integer(i) ); }
	Node& pushint(const string& istr) { return push( Node::Integer(istr) ); }
	Node& pushtoken(const string& t) { return push( Node::Token(t) ); }
	void  pushtokens(const vector<string>& toklist) {
		for (auto& t : toklist)
			pushtoken(t);
	}

	string cmd() const {
		return type == NT_LIST && list.size() > 0 && list.at(0).type == NT_TOKEN
			? list.at(0).tok : NIL_STRING;
	}
	string tokat(int pos) const {
		assert(type == NT_LIST && pos >= 0 && pos < list.size());
		return list[pos].type == NT_TOKEN ? list[pos].tok : NIL_STRING;
	}
	const Node& at(int pos) const { return ((Node*)this)->at(pos); }
	const Node& back()      const { return ((Node*)this)->back(); }
	Node& at(int pos) {
		assert(type == NT_LIST);
		return list.at(pos);
	}
	Node& back() {
		assert(type == NT_LIST && list.size() > 0);
		return list.back();
	}


	void show(int ind=0) const {
		static NODE_TYPE last = NT_NIL;
		// spacing
		string indstr(ind*2, ' ');
		if      (ind == 0) ;  // first (special case)
		else if (last == NT_LIST || type == NT_LIST)  printf("\n%s", indstr.c_str());
		else if (last != NT_NIL)  printf(" ");
		// show
		switch (type) {
		case NT_NIL:         printf("%s ", NIL_STRING.c_str());  break;
		case NT_TOKEN:       printf("%s", tok.c_str());  break;
		case NT_STRLITERAL:  printf("\"%s\"", tok.c_str());  break;
		case NT_INTEGER:     printf("%d", i);  break;
		case NT_LIST:
			last = NT_NIL;
			printf("(");
			for (auto nn : list)
				nn.show(ind+1);
			printf(")");

		}
		last = type;
		if (ind == 0)  printf("\n");  // last (special case)
	}
};