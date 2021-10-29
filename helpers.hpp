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
	// string msg;
	int lno;
	DBError(int _lno) : lno(_lno) {
		error_string = "DougBasic Syntax error, line " + to_string(lno+1);
	}
	virtual const char* what() const noexcept {
		return error_string.c_str();
	}
};



string cleanliteral(const string& s) {
	return (s.length() >= 2 && s[0] == '"' && s.back() == '"') ? s.substr(1, s.length()-2) : s;
}



// ----------------------------------------
// Node syntax-tree structure 
// ----------------------------------------
enum NODE_TYPE {
	NT_NIL=0,
	NT_TOKEN,
	NT_LIST,
	NT_LITERAL,
};

struct Node {
	NODE_TYPE type;
	string tok;
	vector<Node> list;

	Node() : type(NT_NIL) {}
	Node(NODE_TYPE _type) : type(_type) {}
	static Node Token(const string& t) { Node n(NT_TOKEN);  return n.tok = t, n; }
	static Node List() { return Node(NT_LIST); }
	static Node Literal(const string& lit) { Node n(NT_LITERAL);  return n.tok = cleanliteral(lit), n; }

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

	void show(int ind=0) {
		static NODE_TYPE last = NT_NIL;
		// spacing
		string indstr(ind*2, ' ');
		if      (last == NT_LIST || type == NT_LIST)  printf("\n%s", indstr.c_str());
		else if (last != NT_NIL)  printf(" ");
		// show
		switch (type) {
		case NT_NIL:      printf("nil ");  break;
		case NT_TOKEN:    printf("%s", tok.c_str());  break;
		case NT_LITERAL:  printf("\"%s\"", tok.c_str());  break;
		case NT_LIST:
			last = NT_NIL;
			printf("(");
			for (auto nn : list)
				nn.show(ind+1);
			printf(")");

		}
		last = type;
	}
};