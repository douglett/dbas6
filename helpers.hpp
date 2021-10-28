// // ----------------------------------------
// // Various useful functions
// // ----------------------------------------
// #pragma once
// #include <string>
// #include <vector>
// #include <sstream>
// using namespace std;


// struct DBError : std::exception {
// 	string msg, error_string;
// 	int lno;
// 	IBError(const string& _msg="", int _lno=-1) : msg(_msg), lno(_lno) {
// 		error_string = (msg.length() ? msg : "InterBasic runtime exception")
// 			+ (lno >= 0 ? ", line " + to_string(lno+1) : "");
// 	}
// 	virtual const char* what() const noexcept {
// 		return error_string.c_str();
// 	}
// };