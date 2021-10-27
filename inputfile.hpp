// ----------------------------------------
// A mix of dbas-4 and dbas-5 input methods
// ----------------------------------------
#pragma once
#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cassert>
using namespace std;



// ----------------------------------------
// Pattern matcher
// ----------------------------------------

struct InputPattern {
	enum PATTERN_TYPE { PT_NONE=0, PT_LITERAL, PT_RULE };
	struct SubPattern { PATTERN_TYPE ptype = PT_NONE; int record = false; string pattern; };
	string pattern;
	vector<SubPattern> pattlist;

	InputPattern(string _pattern) {
		pattern = _pattern;
		auto vs = split(pattern);
		for (auto patt : vs) {
			SubPattern p;
			if      (patt.substr(0, 1) == "@")  patt = patt.substr(1), p.record = true;
			if      (patt.substr(0, 1) == "'")  patt = patt.substr(1), p.ptype = PT_LITERAL;
			else    p.ptype = PT_RULE;
			p.pattern = patt;
			assert(p.ptype != PT_NONE && p.pattern.length() > 0);
			pattlist.push_back(p);
		}
	}

	int match(const vector<string>& tokens, int start, vector<string>& results) {
		results = {};
		int pos = start;
		for (auto& p : pattlist) {
			switch (p.ptype) {
			case PT_NONE:
				assert("Pattern::match > matching on blank pattern" == NULL);
			case PT_LITERAL:
				if (pos >= tokens.size() || tokens[pos] != p.pattern)  return 0;
				results.push_back(tokens[pos]);
				break;
			case PT_RULE:
				if      (p.pattern == "eol")         return pos >= tokens.size();
				else if (p.pattern == "endl")        return pos >= tokens.size() || tokens[pos][0] == '#';
				else if (pos >= tokens.size())       return 0;
				else if (p.pattern == "comment")     return tokens[pos][0] == '#';
				else if (p.pattern == "identifier")  return is_identifier(tokens[pos]);
				else if (p.pattern == "integer")     return is_integer(tokens[pos]);
				else if (p.pattern == "literal")     return tokens[pos].size() >= 2 && tokens[pos][0] == '"' && tokens[pos].back() == '"';
				else    return assert("Pattern::match > unknown pattern" == NULL), 0;
			}
			pos++;
		}
		return results.size();
	}

	static int is_identifier(const string& s) {
		if (s.length() == 0)  return 0;
		for (int i = 0; i < s.length(); i++)
			if      (i == 0 && !isalpha(s[i]) && s[i] != '_')  return 0;
			else if (i  > 0 && !isalnum(s[i]) && s[i] != '_')  return 0;
		return 1;
	}
	static int is_integer(const string& s) {
		if (s.length() == 0)  return 0;
		for (int i = 0; i < s.length(); i++)
			if (!isdigit(s[i]))  return 0;
		return 1;
	}
	static vector<string> split(const string& str) {
		vector<string> vs;
		stringstream ss(str);
		string s;
		while(ss >> s)  vs.push_back(s);
		return vs;
	}
};



// ----------------------------------------
// Input File handler
// ----------------------------------------

struct InputFile {
	vector<string> lines, tokens, tempresults;
	int lno = 0, pos = 0;

	int load(const string& fname) {
		// reset
		lines = tokens = {};
		lno = pos = 0;
		// load
		fstream fs(fname, ios::in);
		if (!fs.is_open())
			return fprintf(stderr, "error loading file %s\n", fname.c_str()), 1;
		string s;
		while (getline(fs, s))
			lines.push_back(s);
		printf("loaded file: %s (%d)\n", fname.c_str(), (int)lines.size());
		tokenizeline();
		return 0;
	}

	int tokenizeline() {
		// reset & check
		tokens = {};
		pos = 0;
		if (lno < 0 || lno >= lines.size())
			return 0;
		// split
		auto& line = lines[lno];
		string s;
		for (int i=0; i<line.length(); i++) {
			char c = line[i];
			if      (isspace(c))  s = s.length() ? (tokens.push_back(s), "") : "";
			else if (isalnum(c) || c == '_')  s += c;
			else if (c == '"') {
				s = s.length() ? (tokens.push_back(s), "") : "";
				s += c;
				for (++i; i<line.length(); i++) {
					s += line[i];
					if (line[i] == '"')  break;
				}
			}
			else if (c == '#') {
				s = s.length() ? (tokens.push_back(s), "") : "";
				tokens.push_back(line.substr(i));
				break;
			}
			else {
				s = s.length() ? (tokens.push_back(s), "") : "";
				tokens.push_back(string() + c);
			}
		}
		if (s.length())  tokens.push_back(s);
		return 1;
	}

	int nextline() {
		lno++;
		return tokenizeline();
	}


	// helpers
	int eol() const { return pos >= tokens.size(); }
	int eof() const { return lno >= lines.size(); }

	// line pattern matching

	int peek    (const string& pattern) { return peek    (pattern, tempresults); }
	int expect  (const string& pattern) { return expect  (pattern, tempresults); }
	int require (const string& pattern) { return require (pattern, tempresults); }

	int peek(const string& pattern, vector<string>& results) {
		InputPattern pt(pattern);
		return pt.match(tokens, pos, results);
	}
	int expect(const string& pattern, vector<string>& results) {
		int len = peek(pattern, results);
		return pos += len, len;
	}
	int require(const string& pattern, vector<string>& results) {
		int len = peek(pattern, results);
		assert(len > 0);  // TODO: throw some error here
		return pos += len, len;
	}
};