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
struct DBErrorBase : std::exception {
	string error_string;
	virtual const char* what() const noexcept {
		return error_string.c_str();
	}
};
struct DBError : DBErrorBase {
	// string msg;
	int lno;
	DBError(int _lno) : lno(_lno) {
		error_string = "DougBasic Syntax error, line " + to_string(lno+1);
	}
};
struct DBRunError : DBErrorBase {
	DBRunError() {
		error_string = "DougBasic Runtime error";
	}
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
string clean_strliteral(const string& s) {
	return (s.length() >= 2 && s[0] == '"' && s.back() == '"') ? s.substr(1, s.length()-2) : s;
}
vector<string> split(const string& str) {
	vector<string> vs;
	stringstream ss(str);
	string s;
	while(ss >> s)  vs.push_back(s);
	return vs;
}
// string join(const vector<string>& vs)



// ----------------------------------------
// Node syntax-tree structure 
// ----------------------------------------
enum NODE_TYPE {
	NT_NIL=0,
	NT_TOKEN,
	NT_LIST,
	NT_STRLITERAL,
	// NT_INTEGER,
};

const string NIL_STRING="<nil>";

struct Node {
	NODE_TYPE type;
	string tok;
	// int32_t i = 0;
	vector<Node> list;

	Node() : type(NT_NIL) {}
	Node(NODE_TYPE _type) : type(_type) {}
	static Node Token(const string& t) { Node n(NT_TOKEN);  return n.tok = t, n; }
	static Node List() { return Node(NT_LIST); }
	static Node Literal(const string& lit) { Node n(NT_STRLITERAL);  return n.tok = clean_strliteral(lit), n; }

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