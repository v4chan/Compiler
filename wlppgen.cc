// Starter code for CS241 assignments 9-11
//
// C++ translation by Simon Parent (Winter 2011),
// based on Java code by Ondrej Lhotak,
// which was based on Scheme code by Gord Cormack.
// Modified July 3, 2012 by Gareth Davies
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
using namespace std;

// The set of terminal symbols in the WLPP grammar.
const char *terminals[] = {
	"BOF", "BECOMES", "COMMA", "ELSE", "EOF", "EQ", "GE", "GT", "ID",
	"IF", "INT", "LBRACE", "LE", "LPAREN", "LT", "MINUS", "NE", "NUM",
	"PCT", "PLUS", "PRINTLN", "RBRACE", "RETURN", "RPAREN", "SEMI",
	"SLASH", "STAR", "WAIN", "WHILE", "AMP", "LBRACK", "RBRACK", "NEW",
	"DELETE", "NULL"
};
int isTerminal(const string &sym) {
	int idx;
	for(idx=0; idx<sizeof(terminals)/sizeof(char*); idx++)
		if(terminals[idx] == sym) return 1;
	return 0;
}

// Data structure for storing the parse tree.
class tree {
	public:
		string rule;
		vector<string> tokens;
		vector<tree*> children;
		~tree() { for(int i=0; i<children.size(); i++) delete children[i]; }
};

// Call this to display an error message and exit the program.
void bail(const string &msg) {
	// You can also simply throw a string instead of using this function.
	throw string(msg);
}

// Read and return wlppi parse tree.
tree *readParse(const string &lhs) {
	// Read a line from standard input.
	string line;
	getline(cin, line);
	if(cin.fail())
		bail("ERROR: Unexpected end of file.");
	tree *ret = new tree();
	// Tokenize the line.
	stringstream ss;
	ss << line;
	while(!ss.eof()) {
		string token;
		ss >> token;
		if(token == "") continue;
		ret->tokens.push_back(token);
	}
	// Ensure that the rule is separated by single spaces.
	for(int idx=0; idx<ret->tokens.size(); idx++) {
		if(idx>0) ret->rule += " ";
		ret->rule += ret->tokens[idx];
	}
	// Recurse if lhs is a nonterminal.
	if(!isTerminal(lhs)) {
		for(int idx=1/*skip the lhs*/; idx<ret->tokens.size(); idx++) {
			ret->children.push_back(readParse(ret->tokens[idx]));
		}
	}
	return ret;
}

tree *parseTree;

map<string, string> symT;
map<string, string>::iterator it = symT.begin();

// Compute symbols defined in t.
void genSymbols(tree *t) {
	for (it = symT.begin(); it != symT.end(); it++) {
		cerr << it->first << " " << it->second << endl;
	}
}

string getType(tree *t) {
	if (t->rule == "expr term") {
		return getType(t->children[0]);
	}
	else if (t->rule == "expr expr PLUS term") {
		string l = getType(t->children[0]);
		string r = getType(t->children[2]);
		if (l == "int" && r == "int") {
			return "int";
		}
		else if (l == "int*" && r == "int") {
			return "int*";
		}
		else if (l == "int" && r == "int*") {
			return "int*";
		}
		else if (l == "int*" && r == "int*") {
			return "ERROR: cannot add two pointers";
		}
	}
	else if (t->rule == "expr expr MINUS term") {
		string l = getType(t->children[0]);
		string r = getType(t->children[2]);
		if (l == "int" && r == "int") {
			return "int";
		}
		else if (l == "int*" && r == "int") {
			return "int*";
		}
		else if (l == "int*" && r == "int*") {
			return "int";
		}
		else if (l == "int" && r == "int*") {
			return "ERROR: cannot subtract an int from a pointer";
		}
	}
	else if (t->rule == "term factor") {
		return getType(t->children[0]);
	}
	else if (t->rule == "term term STAR factor") {
		string l = getType(t->children[0]);
		string r = getType(t->children[2]);
		if (l == "int" && r == "int") {
			return "int";
		}
		else {
			return "ERROR: multiplication";
		}
	}
	else if (t->rule == "term term SLASH factor") {
		string l = getType(t->children[0]);
		string r = getType(t->children[2]);
		if (l == "int" && r == "int") {
			return "int";
		}
		else {
			return "ERROR: division";
		}
	}
	else if (t->rule == "term term PCT factor") {
		string l = getType(t->children[0]);
		string r = getType(t->children[2]);
		if (l == "int" && r == "int") {
			return "int";
		}
		else {
			return "ERROR: remainder";
		}
	}
	else if (t->rule == "factor ID") {
		return symT[t->children[0]->tokens[1]];
	}
	else if (t->rule == "factor NUM") {
		return "int";
	}
	else if (t->rule == "factor NULL") {
		return "int*";
	}
	else if (t->rule == "factor LPAREN expr RPAREN") {
		return getType(t->children[1]);
	}
	else if (t->rule == "factor AMP lvalue") {
		string factorType = getType(t->children[1]);
		if (factorType == "int") {
			return "int*";
		}
		else {
			return "ERROR: not &int";
		}
	}
	else if (t->rule == "factor STAR factor") {
		string factorType = getType(t->children[1]);
		if (factorType == "int*") {
			return "int";
		}
		else {
			return "ERROR: cannot dereference a non-pointer";
		}
	}
	else if (t->rule == "factor NEW INT LBRACK expr RBRACK") {
		string factorType = getType(t->children[3]);
		if (factorType == "int") {
			return "int*";
		}
		else {
			return "ERROR: not new int [int]";
		}
	}
	else if (t->rule == "lvalue ID") {
		return symT[t->children[0]->tokens[1]];
	}
	else if (t->rule == "lvalue STAR factor") {
		string factorType = getType(t->children[1]);
		if (factorType == "int*") {
			return "int";
		}
		else {
			return "ERROR: cannot dereference a non-pointer";
		}
	}
	else if (t->rule == "lvalue LPAREN lvalue RPAREN") {
		return getType(t->children[1]);
	}
}

bool wellType(tree *t) {
	if (t->rule == "test expr EQ expr") {
		string l = getType(t->children[0]);
		string r = getType(t->children[2]);
		if (l == "int" && r == "int") {
			return true;
		}
		else if (l == "int*" && r == "int*") {
			return true;
		}
		else {
			return false;
		}
	}
	else if (t->rule == "test expr NE expr") {
		string l = getType(t->children[0]);
		string r = getType(t->children[2]);
		if (l == "int" && r == "int") {
			return true;
		}
		else if (l == "int*" && r == "int*") {
			return true;
		}
		else {
			return false;
		}
	}
	else if (t->rule == "test expr LT expr") {
		string l = getType(t->children[0]);
		string r = getType(t->children[2]);
		if (l == "int" && r == "int") {
			return true;
		}
		else if (l == "int*" && r == "int*") {
			return true;
		}
		else {
			return false;
		}
	}
	else if (t->rule == "test expr LE expr") {
		string l = getType(t->children[0]);
		string r = getType(t->children[2]);
		if (l == "int" && r == "int") {
			return true;
		}
		else if (l == "int*" && r == "int*") {
			return true;
		}
		else {
			return false;
		}
	}
	else if (t->rule == "test expr GE expr") {
		string l = getType(t->children[0]);
		string r = getType(t->children[2]);
		if (l == "int" && r == "int") {
			return true;
		}
		else if (l == "int*" && r == "int*") {
			return true;
		}
		else {
			return false;
		}
	}
	else if (t->rule == "test expr GT expr") {
		string l = getType(t->children[0]);
		string r = getType(t->children[2]);
		if (l == "int" && r == "int") {
			return true;
		}
		else if (l == "int*" && r == "int*") {
			return true;
		}
		else {
			return false;
		}
	}
	else if (t->rule == "statements") {
		return true;
	}
	else if (t->rule == "statements statements statement") {
		return wellType(t->children[1]);
	}
	else if (t->rule == "statement lvalue BECOMES expr SEMI") {
		string l = getType(t->children[0]);
		string r = getType(t->children[2]);
		if (l == "int" && r == "int") {
			return true;
		}
		else if (l == "int*" && r == "int*") {
			return true;
		}
		else {
			return false;
		}
	}
	else if (t->rule == "statement IF LPAREN test RPAREN LBRACE statements RBRACE ELSE LBRACE statements RBRACE") {
		bool a = wellType(t->children[2]);
		bool b = wellType(t->children[5]);
		bool c = wellType(t->children[9]);
		if (a && b && c) {
			return true;
		}
		else {
			return false;
		}
	}
	else if (t->rule == "statement WHILE LPAREN test RPAREN LBRACE statements RBRACE") {
		bool a = wellType(t->children[2]);
		bool b = wellType(t->children[5]);
		if (a && b) {
			return true;
		}
		else {
			return false;
		}
	}
	else if (t->rule == "statement PRINTLN LPAREN expr RPAREN SEMI") {
		string expr = getType(t->children[2]);
		if (expr == "int") {
			return true;
		}
		else {
			return false;
		}
	}
	else if (t->rule == "statement DELETE LBRACK RBRACK expr SEMI") {
		string expr = getType(t->children[3]);
		if (expr == "int*") {
			return true;
		}
		else {
			return false;
		}
	}
}

int flag = 0;
// Generate the code for the parse tree t.
void genCode(tree *t) {
	if (t->rule == "procedure INT WAIN LPAREN dcl COMMA dcl RPAREN LBRACE dcls statements RETURN expr SEMI RBRACE") {
		if (t->children[5]->children[0]->rule == "type INT STAR") {
			cerr << "ERROR: Second parameter in int wain is not an int" << endl;
		}
	}
	if (t->rule == "dcl type ID") {
		string type = "";
		if (t->children[0]->rule == "type INT") {
			type = "int";
		}
		else if (t->children[0]->rule == "type INT STAR") { 
			type = "int*";
		}
		string name = t->children[1]->tokens[1];
		if (symT.find(name) == symT.end()) {
			symT[name] = type;
		}
		else {
			cerr << "ERROR: duplicate declaration: " << name << endl;
		}
	}
	else if (t->rule == "dcls dcls dcl BECOMES NUM SEMI") {
		if (t->children[1]->children[0]->rule == "type INT STAR") {
			cerr << "ERROR: BECOMES not well typed" << endl;
		}
	}
	else if (t->rule == "dcls dcls dcl BECOMES NULL SEMI") {
		if (t->children[1]->children[0]->rule == "type INT") {
			cerr << "ERROR: BECOMES not well typed" << endl;
		}
	}
	else if (t->tokens.size() == 2 && (t->rule.substr(0,6) == "factor" || t->rule.substr(0,6) == "lvalue")) {
		string name = t->children[0]->tokens[1];
		if (symT.find(name) == symT.end() && t->children[0]->tokens[0] != "NUM") {
			cerr << "ERROR: missing declaration: " << name << endl;
		}
	}
	else if (t->rule.size() >= 4 && (t->rule.substr(0,4) == "expr" || t->rule.substr(0,6) == "lvalue")) {
		if (flag == 1) {
			string rtype = getType(t);
			if (rtype != "int") {
				cerr << "ERROR: incorrect return type" << endl;
				flag = 0;
			}
		}
		string type = getType(t);
		if (type.size() >= 5 && type.substr(0,5) == "ERROR") {
			cerr << type << endl;
		}
	}
	else if (t->rule.size() >= 4 && (t->rule.substr(0,4) == "test" || t->rule.substr(0,9) == "statement")) {
		bool type = wellType(t);
		if (type == false) {
			cerr << "ERROR: not well typed" << endl;
		}
	}
	else if (t->rule.size() >= 6 && t->rule.substr(0,6) == "RETURN") {
		flag = 1;
	}
	for (int i = 0; i < t->tokens.size(); i++) {
		if (t->children.size() == 0) {
			break;
		}
		else {
			if (i < t->children.size()) {
				genCode(t->children[i]);
			}
			else {
				break;
			}
		}
	}
}

int main() {
	// Main program.
	try {
		parseTree = readParse("S");
		genCode(parseTree);
		//genSymbols(parseTree);
	} catch(string msg) {
		cerr << msg << endl;
	}
	if (parseTree) delete parseTree;
	return 0;
}
