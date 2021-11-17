// ----------------------------------------
// Standard library functions
// ----------------------------------------
#pragma once
#include "helpers.hpp"
// #include "inputfile.hpp"
using namespace std;


struct StdLib {
	// void test(InputFile& p) {
	// 	p.eol();
	// }

	void appendto(Node& fndef) {
		// push (temp)
		auto& fn = fndef.pushlist();
			fn.pushtokens({ "function", "std::push" });
			auto& args = fn.pushcmdlist("args");
				auto& a = args.pushlist();
					a.pushtokens({ "dim", "arr", "string[]" });
				auto& b = args.pushlist();
					b.pushtokens({ "dim", "glue", "string" });
	}
};