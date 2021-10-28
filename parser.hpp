// ----------------------------------------
// Basic parser main
// ----------------------------------------
#pragma once
#include "inputfile.hpp"
using namespace std;


// struct Node {
// 	string val;
// 	vector<Node> list;

// 	Node& push(const Node& n) { list.push_back(n);  return list.back(); }
// 	Node  pop () { if (list.size() == 0) return {};  Node n = list.back();  list.pop_back();  return n; }
// 	void  show(int indent=0) const;
// };

// void Node::show(int indent) const {
// 	string i(indent*4, ' ');
// 	printf("%s%s%s\n", i.c_str(), val.c_str(), list.size() ? " :: " : "" );
// 	for (auto& n : list)
// 		n.show(indent+1);
// }


enum NODE_TYPE {
	NT_NIL=0,
	NT_TOKEN,
	NT_LIST
};

struct Node {
	NODE_TYPE type;
	string tok;
	vector<Node> list;

	Node() : type(NT_NIL) {}
	Node(NODE_TYPE _type) : type(_type) {}
	static Node Token(const string& t) { Node n(NT_TOKEN);  return n.tok = t, n; }
	static Node List() { return Node(NT_LIST); }

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
	void   pushtokens(const vector<string>& toklist) {
		for (auto& t : toklist)
			pushtoken(t);
	}

	void show(int ind=0) {
		static NODE_TYPE last = NT_NIL;
		string indstr(ind*2, ' ');
		switch (type) {
		case NT_NIL:    printf("nil ");  break;
		case NT_TOKEN:  printf("%s ", tok.c_str());  break;
		case NT_LIST:
			if (last != NT_LIST && ind > 0)  printf("\n%s", indstr.c_str() );
			printf("(");
			for (auto nn : list)
				nn.show(ind+1);
			printf(")\n%s", indstr.c_str() );
		}
		last = type;
	}
};



struct Parser : InputFile {
	Node prog;

	void p_program() {
		prog = Node::List();
		prog.pushtoken("program");
		p_section("type", prog);
	}

	void p_section(const string& section, Node& p) {
		Node& nn = p.pushlist();
		nn.pushtoken("section");
		while (!eof())
			if      (expect("endl"))  nextline();
			else if (section == "type"      && peek("'type"))      p_type(nn);
			// else if (section == "dimglobal" && peek("'dim"))       p_dimglobal();
			// else if (section == "function"  && peek("'function"))  p_function();
			else    break;
	}

	void p_type(Node& p) {
		require("'type @identifier endl");
		string tname = presults.at(0);
		Node& nn = p.pushlist();
		nn.pushtokens({ "type", tname });
		nextline();
		while (!eof()) {
			if      (expect("endl")) ;
			else if (expect("'dim @identifier @identifier endl"))
				nn.pushlist().pushtokens({ "dim", presults.at(0), presults.at(1) });
			else if (expect("'dim @identifier endl"))
				nn.pushlist().pushtokens({ "dim", "int", presults.at(0) });
			else    break;
			nextline();
		}
		require("'end 'type endl");
		// printf("---\n");
		// nn.show();
		// printf("---\n");
	}
};