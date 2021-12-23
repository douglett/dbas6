// ----------------------------------------
// Various useful functions
// ----------------------------------------
#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
using namespace std;



// ----------------------------------------
// Error codes
// ----------------------------------------
enum DB_PARSE_ERROR {
	// generic errors
	ERR_SYNTAX_ERROR = 2,
	ERR_UNEXPECTED_EOL,
	ERR_EXPECTED_EOF,
	// expressions
	ERR_UNKNOWN_ATOM,
	ERR_EXPECTED_INT,
	ERR_UNMATCHED_TYPES,
	ERR_UNDEFINED_VARIABLE,
	ERR_UNDEFINED_TYPE,
	ERR_UNDEFINED_FUNCTION,
	ERR_ARGUMENT_COUNT_MISMATCH,
	ERR_ARGUMENT_INDEX_MISSING,
	// commands
	ERR_UNKNOWN_COMMAND,
	// definitions
	ERR_UNEXPECTED_KEYWORD,
	ERR_ALREADY_DEFINED,
	ERR_CIRCULAR_DEFINITION,
};
string error_message(DB_PARSE_ERROR err) {
	switch (err) {
	case ERR_SYNTAX_ERROR:                 return "ERR_SYNTAX_ERROR";
	case ERR_UNEXPECTED_EOL:               return "ERR_UNEXPECTED_EOL";
	case ERR_EXPECTED_EOF:                 return "ERR_EXPECTED_EOF";
	case ERR_UNKNOWN_ATOM:                 return "ERR_UNKNOWN_ATOM";
	case ERR_EXPECTED_INT:                 return "ERR_EXPECTED_INT";
	case ERR_UNMATCHED_TYPES:              return "ERR_UNMATCHED_TYPES";
	case ERR_UNDEFINED_VARIABLE:           return "ERR_UNDEFINED_VARIABLE";
	case ERR_UNDEFINED_TYPE:               return "ERR_UNDEFINED_TYPE";
	case ERR_UNDEFINED_FUNCTION:           return "ERR_UNDEFINED_FUNCTION";
	case ERR_ARGUMENT_COUNT_MISMATCH:      return "ERR_ARGUMENT_COUNT_MISMATCH";
	case ERR_ARGUMENT_INDEX_MISSING:       return "ERR_ARGUMENT_INDEX_MISSING";
	case ERR_UNKNOWN_COMMAND:              return "ERR_UNKNOWN_COMMAND";
	case ERR_UNEXPECTED_KEYWORD:           return "ERR_UNEXPECTED_KEYWORD";
	case ERR_ALREADY_DEFINED:              return "ERR_ALREADY_DEFINED";
	case ERR_CIRCULAR_DEFINITION:          return "ERR_CIRCULAR_DEFINITION";
	}
	return "UNKNOWN_ERROR_CODE_"+to_string(err);
}


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
	DBParseError(DB_PARSE_ERROR err, int lno, const string& ctok) {
		error_string = "Error: " + error_message(err)
			+ "  :: line " + to_string(lno+1)
			+ ", at token '" + ctok + "'";
	}
};
// struct DBRunError : DBError {
// 	DBRunError() {
// 		error_string = "DougBasic Runtime error";
// 	}
// };
// struct DBCtrl : std::exception { };
// struct DBCtrlReturn : DBCtrl { };
// struct DBCtrlBreak : DBCtrl {
// 	int level;
// 	DBCtrlBreak(int lv) : level(lv) { }
// };
// struct DBCtrlContinue : DBCtrl {
// 	int level;
// 	DBCtrlContinue(int lv) : level(lv) { }
// };


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
	if      (is_arraytype(s))   return s.substr(0, s.length()-2);
	else if (is_reftype(s))     return s.substr(0, s.length()-1);
	else if (is_arrreftype(s))  return s.substr(0, s.length()-1);
	else    return s;
}
string clean_strliteral(const string& s) {
	return is_strliteral(s) ? s.substr(1, s.length()-2) : s;
}
string chomp(const string& s) {
	int i = 0, j = s.length() - 1;
	for ( ; i < s.length(); i++)
		if (!isspace(s[i]))  break;
	for ( ; j >= 0; j--)
		if (!isspace(s[j]))  { j++; break; }
	return s.substr(i, j-i);
}
vector<string> splitws(const string& str) {
	vector<string> vs;
	string s;
	for (auto c : str)
		if   (isspace(c)) { if (s.length())  vs.push_back(s), s = ""; }
		else              { s += c; }
	if (s.length())  vs.push_back(s);
	return vs;
}
string join(const vector<string>& vs, const string& glue=" ") {
	string str;
	for (int i = 0; i < vs.size(); i++)
		str += (i > 0 ? glue : "") + vs[i];
	return str;
}



// ----------------------------------------
// Node syntax-tree structure 
// ----------------------------------------
// enum NODE_TYPE {
// 	NT_NIL=0,
// 	NT_TOKEN,
// 	NT_LIST,
// 	NT_STRLITERAL,
// 	NT_INTEGER,
// };

// const string NIL_STRING="<nil>";

// struct Node {
// 	NODE_TYPE type = NT_NIL;
// 	string tok;
// 	int32_t i = 0;
// 	vector<Node> list;

// 	Node() {}
// 	Node(NODE_TYPE _type) : type(_type) {}
// 	static Node Token(const string& t) { Node n(NT_TOKEN);  return n.tok = t, n; }
// 	static Node List() { return Node(NT_LIST); }
// 	static Node CmdList(const string& t) { Node n(NT_LIST);  return n.pushtoken(t), n; }
// 	static Node Literal(const string& lit) { Node n(NT_STRLITERAL);  return n.tok = clean_strliteral(lit), n; }
// 	static Node Integer(int32_t i) { Node n(NT_INTEGER);  return n.i = i, n; }
// 	static Node Integer(const string& istr) { Node n(NT_INTEGER);  return n.i = stoi(istr), n; }

// 	Node  pop() {
// 		assert(type == NT_LIST && list.size() > 0);
// 		auto n = list.back();
// 		return list.pop_back(), n;
// 	}
// 	Node& push(const Node& n) {
// 		assert(type == NT_LIST);
// 		list.push_back( n );
// 		return list.back();		
// 	}
// 	Node& pushlist() { return push( Node::List() ); }
// 	Node& pushcmdlist(const string& t) { return push( Node::CmdList(t) ); }
// 	Node& pushliteral(const string& lit) { return push( Node::Literal(lit) ); }
// 	Node& pushint(int32_t i) { return push( Node::Integer(i) ); }
// 	Node& pushint(const string& istr) { return push( Node::Integer(istr) ); }
// 	Node& pushtoken(const string& t) { return push( Node::Token(t) ); }
// 	void  pushtokens(const vector<string>& toklist) {
// 		for (auto& t : toklist)
// 			pushtoken(t);
// 	}

// 	string cmd() const {
// 		return type == NT_LIST && list.size() > 0 && list.at(0).type == NT_TOKEN
// 			? list.at(0).tok : NIL_STRING;
// 	}
// 	string tokat(int pos) const {
// 		assert(type == NT_LIST && pos >= 0 && pos < list.size());
// 		return list[pos].type == NT_TOKEN ? list[pos].tok : NIL_STRING;
// 	}
// 	const Node& at(int pos) const { return ((Node*)this)->at(pos); }
// 	const Node& back()      const { return ((Node*)this)->back(); }
// 	Node& at(int pos) {
// 		assert(type == NT_LIST);
// 		return list.at(pos);
// 	}
// 	Node& back() {
// 		assert(type == NT_LIST && list.size() > 0);
// 		return list.back();
// 	}


// 	void show(int ind=0) const {
// 		static NODE_TYPE last = NT_NIL;
// 		// spacing
// 		string indstr(ind*2, ' ');
// 		if      (ind == 0) ;  // first (special case)
// 		else if (last == NT_LIST || type == NT_LIST)  printf("\n%s", indstr.c_str());
// 		else if (last != NT_NIL)  printf(" ");
// 		// show
// 		switch (type) {
// 		case NT_NIL:         printf("%s ", NIL_STRING.c_str());  break;
// 		case NT_TOKEN:       printf("%s", tok.c_str());  break;
// 		case NT_STRLITERAL:  printf("\"%s\"", tok.c_str());  break;
// 		case NT_INTEGER:     printf("%d", i);  break;
// 		case NT_LIST:
// 			last = NT_NIL;
// 			printf("(");
// 			for (auto nn : list)
// 				nn.show(ind+1);
// 			printf(")");

// 		}
// 		last = type;
// 		if (ind == 0)  printf("\n");  // last (special case)
// 	}
// };