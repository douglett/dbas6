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
	enum PATTERN_TYPE { PT_NONE=0, PT_WORD, PT_RULE };
	struct SubPattern { PATTERN_TYPE ptype = PT_NONE; int record = false; string pattern; };
	typedef  vector<string>  Results;
	string pattern;
	vector<SubPattern> pattlist;

	InputPattern(string _pattern) {
		pattern = _pattern;
		auto vs = split(pattern);
		for (auto patt : vs) {
			SubPattern p;
			if      (patt.substr(0, 1) == "@")  patt = patt.substr(1), p.record = true;
			if      (patt.substr(0, 1) == "'")  patt = patt.substr(1), p.ptype = PT_WORD;
			else    p.ptype = PT_RULE;
			p.pattern = patt;
			assert(p.ptype != PT_NONE && p.pattern.length() > 0);
			pattlist.push_back(p);
		}
	}

	int match(const vector<string>& tokens, int start, Results& results) {
		results = {};
		int pos = start, ismatch = 0;
		// loop through each sub-pattern and match
		for (auto& p : pattlist) {
			switch (p.ptype) {
			case PT_NONE:
				assert("Pattern::match > matching on blank pattern" == NULL);
			case PT_WORD:
				if (pos >= tokens.size() || tokens[pos] != p.pattern)  return 0;  // cancel (match nothing)
				if (p.record)  results.push_back( tokens[pos] );
				break;
			case PT_RULE:
				// built in rules
				ismatch = 0;
				if      (p.pattern == "eol")         ismatch = pos >= tokens.size();
				else if (p.pattern == "endl")        ismatch = pos >= tokens.size() || tokens[pos][0] == '#';
				else if (pos >= tokens.size())       ismatch = 0;
				else if (p.pattern == "comment")     ismatch = tokens[pos][0] == '#';
				else if (p.pattern == "identifier")  ismatch = is_identifier(tokens[pos]);
				else if (p.pattern == "integer")     ismatch = is_integer(tokens[pos]);
				else if (p.pattern == "literal")     ismatch = is_strliteral(tokens[pos]);
				else    assert("Pattern::match > unknown pattern" == NULL);
				if (!ismatch)  return 0;  // cancel (match nothing)
				if (p.record)  results.push_back( pos < tokens.size() ? tokens[pos] : "<EOL>" );
				break;
			}
			pos++;  // next
		}
		// return total number of tokens eaten
		return pattlist.size();
	}

	int matchrule(const vector<string>& tokens, int pos, const SubPattern& p) {
		if      (p.pattern == "eol")         return pos >= tokens.size();
		else if (p.pattern == "endl")        return pos >= tokens.size() || tokens[pos][0] == '#';
		else if (pos >= tokens.size())       return 0;
		else if (p.pattern == "comment")     return tokens[pos][0] == '#';
		else if (p.pattern == "identifier")  return is_identifier(tokens[pos]);
		else if (p.pattern == "integer")     return is_integer(tokens[pos]);
		else if (p.pattern == "literal")     return tokens[pos].size() >= 2 && tokens[pos][0] == '"' && tokens[pos].back() == '"';
		else    return assert("Pattern::match > unknown pattern" == NULL), 0;
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
	static int is_strliteral(const string& s) {
		return s.size() >= 2 && s[0] == '"' && s.back() == '"';
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
	// typedef  InputPattern::Results  Results;
	vector<string> lines, tokens;
	InputPattern::Results presults;
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

	int loadstring(const string& program) {
		// reset
		lines = tokens = {};
		lno = pos = 0;
		// load
		stringstream ss(program);
		string s;
		while (getline(ss, s))
			lines.push_back(s);
		printf("loaded program string.\n");
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

	int peek    (const string& pattern) { return peek    (pattern, presults); }
	int expect  (const string& pattern) { return expect  (pattern, presults); }
	int require (const string& pattern) { return require (pattern, presults); }

	int peek(const string& pattern, InputPattern::Results& results) {
		InputPattern pt(pattern);
		return pt.match(tokens, pos, results);
	}
	int expect(const string& pattern, InputPattern::Results& results) {
		int len = peek(pattern, results);
		return pos += len, len;
	}
	int require(const string& pattern, InputPattern::Results& results) {
		int len = peek(pattern, results);
		assert(len > 0 || "Required pattern not found" == NULL);  // TODO: throw some error here
		return pos += len, len;
	}
};