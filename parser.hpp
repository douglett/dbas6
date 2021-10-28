// ----------------------------------------
// Basic parser main
// ----------------------------------------
#pragma once
#include "inputfile.hpp"
using namespace std;


struct Node {
	string val;
	vector<Node> list;

	Node& push(const Node& n) { list.push_back(n);  return list.back(); }
	Node  pop () { if (list.size() == 0) return {};  Node n = list.back();  list.pop_back();  return n; }
	void  show(int indent=0) const;
};

void Node::show(int indent) const {
	string i(indent*4, ' ');
	printf("%s%s%s\n", i.c_str(), val.c_str(), list.size() ? " :: " : "" );
	for (auto& n : list)
		n.show(indent+1);
}


// struct Node2 {
// 	enum NODE_TYPE {
// 		NT_NIL=0,
// 		NT_TOKEN,
// 		NT_LIST
// 	};

// 	NODE_TYPE type;
// 	string tok;
// 	vector<Node> list;

// 	static Node Token(const string& s) {
// 		return { .type=NT_TOKEN, .tok=s };
// 	}
// 	static Node List() {
// 		return { .type=NT_LIST };
// 	}

// 	Node& pushtokens(const vector<string>& toklist) {
// 		assert(type == NT_LIST);
// 		toklist.push_back( Node2::List() );
// 		auto& nl = toklist.back();
// 		for (auto& t : toklist)
// 			nl.list.push_back( Node2::Token(t) );
// 		return nl;
// 	}
// };


struct Parser : InputFile {
	void p_type() {
		Node n = { "type" };
		require("'type @identifier endl");
		string tname = presults.at(0);
		// printf("type: %s\n", tname.c_str());
		n.push({ "name", { {tname} } });

		nextline();
		while (!eof()) {
			if      (expect("endl")) ;
			else if (expect("'dim @identifier @identifier endl"))
				// printf("type-member: %s, %s %s\n", tname.c_str(), presults.at(0).c_str(), presults.at(1).c_str()),
				n.push({ "member", { {presults.at(0)}, {presults.at(1)} }});
			else if (expect("'dim @identifier endl"))
				// printf("type-member: %s, int %s\n", tname.c_str(), presults.at(0).c_str()),
				// n2.pushtokens({ "member", "int", presults.at(0) });
				n.push({ "member", { {"int"}, {presults.at(0)} }});
			else    break;
			nextline();
		}
		require("'end 'type endl");

		n.show();
	}
};