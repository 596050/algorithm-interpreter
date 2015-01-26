#ifdef COMPONENT_PARSER_H
// do not include this file directly.
// use parser.h instead.

/************************************
* Implementation: class Error      **
*************************************/

Error::Error(String cd, String fg, bufferIndex ln, int s) {
	this->_code = cd;
	this->_flag = fg;
	this->_line = ln;
	this->_severity = s;
}
Error::Error(const Error& e) {
	this->_code = e._code;
	this->_flag = e._flag;
	this->_line = e._line;
	this->_severity = e._severity;
}
Error& Error::operator= (const Error& e) {
	this->_code = e._code;
	this->_flag = e._flag;
	this->_line = e._line;
	this->_severity = e._severity;
	return *this;
}

String Error::code() const { return this->_code; }
String Error::flag() const { return this->_flag; }
bufferIndex Error::lineNumber() const { return this->_line; }
int Error::severity() const { return this->_severity; }

String Error::message() const {
	__SIZETYPE i;
	String ret;
	ret = ret + "Line " + integerToString(this->_line) + ": ";

	if (this->_severity == FATAL) ret += "FATAL Error: ";
	else if (this->_severity == ERROR) ret += "Error: ";
	else if (this->_severity == WARNING) ret += "Warning: ";

	i = errorCodes.indexOf(this->_code);
	if (i >= 0) ret += errorDesc[i];
	ret += this->_flag;

	return ret;
}

bool Error::setLineNumber(bufferIndex ln) { this->_line = ln; }
/*** END implementation: class Error ***/

bool importErrorCodes(ifstream& ecreader) {
	if (!ecreader) return false;
	String buff; Vector<String> vs;

	errorCodes.clear(); errorDesc.clear();
	while (!ecreader.eof()) {
		buff.get(ecreader, '\n');
		if (buff.substr(0, 1) != "#" && buff.indexOf(":") > 0) {
			vs = strsplit(buff, ":");
			errorCodes.pushback(vs[0].trim());
			errorDesc.pushback(vs[1]);
		}
	}
	ecreader.close();
	return true;
}

// include the implementation of classes:
//     Variable, VariableScope, Function
#include "variables.cpp"

/*************************************
* Implementation: class Parser
*************************************/

// Construction, and setup.
Parser::Parser(Lexer *ref, String args) {
	this->lexer = ref;
	this->status = PENDING;
}

Parser::~Parser() {
	functions.clear();
	errors.clear();
	output.clear();
}

/*** Parser Interface ***/
bool Parser::sendError(Error e) {
	this->errors.pushback(e);
	if (e.severity() == FATAL) {
		// a fatal error, so stop parsing further.
		this->status = FAILED;
	}
	return true;
}

Function Parser::getFunction(String id) {
	Function f;
	for (__SIZETYPE i = 0; i < this->functions.size(); i++) {
		f = this->functions[i];
		if (f.id() == id) return f;
	}
	return f;
}

/**** Parsing Procedures ****/
bool Parser::parseSource() {
	// some error checks before proceeding:
	
}
RPN Parser::parseDeclaration(tokenType type) {
	// procedure to parse array and hash table primitives.
	// errors can be directly caught. carefully call converting to RPN.
	return RPN();
}
RPN Parser::parseStatement() {
	return RPN();
	
}
/**** Static Parsing Procedures ****/
RPN Parser::expressionToRPN(Infix args) {
	if (args.empty()) return RPN();

	RPN output;
	RPN preChange, postChange;
	Token current, prevtok;
	Stack<Token> operstack;

	static Token subscriptOp = Lexer::toToken("[]"),
	             ternaryOp = Token("?:", OPERATOR, MULTINARYOP),
	             statementEnd = Lexer::toToken(";"),
	             pluseqTok = Lexer::toToken("+="),
	             minuseqTok = Lexer::toToken("-="),
	             oneTok = Lexer::toToken("1");
	
	static Token funcInvoke = Token("@invoke", DIRECTIVE, FUNCTION);

	args.push(statementEnd);
	Stack<__SIZETYPE> funcarglist;
	
	while (args.pop(current)) {

		if (current.value() == "\n" || current.value() == ";") {
			while (operstack.pop(current)) {
				// check for unbalanced ( and [
				if (current.value() == "(" || current.value() == "[") this->sendError(Error("p1", current.value(), current.lineNumber()));
				output.push(current);
			}
			current = statementEnd;
			output.push(statementEnd);
		}

		if (current.type() == IDENTIFIER) {
			// check whether it is a function:
			if (!args.empty() && args.front().value() == "(") {
				current.setSubtype(FUNCTION);
			} else { // is a variable/property
				output.push(current);
			}
		}
		if (current.subtype() == FUNCTION) {
			operstack.push(current);
			Token tmp;
			if (!args.pop(tmp) || tmp.value() != "(") this->sendError(Error("p3.1", "", current.lineNumber())); // function call should be succeeded by a (
			operstack.push(tmp);
			
			if (!args.empty() && args.front().value() == ")") {
				// zero argument function call:
				funcarglist.push(0);
			} else {
				funcarglist.push(1);
			}
		}

		if (current.type() == PUNCTUATOR) {
			Token tmp;

			if (current.value() == "(" || current.value() == "[") {
				operstack.push(current);
			}
			else if (current.value() == ")") {
				while (operstack.pop(tmp)) {
					if (tmp.value() == "(") {
						if (!operstack.empty() && operstack.top().subtype() == FUNCTION) {
							Token func;
							operstack.pop(func);
							output.push(func);
							
							__SIZETYPE argnum;
							if (funcarglist.pop(argnum)) output.push(toArgsToken(argnum));
							else {
								// unable to find number of arguments: arglist is empty.
								// should not happen.
								this->sendError(Error("p3.2", "@args", tmp.lineNumber())); 
							}
							
							if (!operstack.empty() && operstack.top().value() == ".") {
								// dereference the method.
								Token deref;
								operstack.pop(deref);
								output.push(deref);
							}
							output.push(funcInvoke);
						}
						break;
					}
					output.push(tmp);
				}
				if (tmp.value() != "(") {
					this->sendError(Error("p1", ")", tmp.lineNumber())); // unbalanced )
					DEBUG(tmp.value());
				}
			}
			else if (current.value() == "]") {
				while (operstack.pop(tmp)) {
					if (tmp.value() == "[") break;
					output.push(tmp);
				}
				if (tmp.value() != "[") this->sendError(Error("p1", "]", tmp.lineNumber())); // unbalanced ]
				output.push(subscriptOp);
			}
			else if (current.value() == ",") {
				while (!operstack.empty()) {
					if (operstack.top().value() == "(") break;
					operstack.pop(tmp);
					output.push(tmp);
				}
				if (!funcarglist.empty()) funcarglist.top()++;
			}
			else if (current.value() == ":") {
				while (operstack.pop(tmp)) {
					if (tmp.value() == "?") break;
					output.push(tmp);
				}
				if (tmp.value() != "?") this->sendError(Error("p2", ": without a ?", tmp.lineNumber())); // unexpected : without a ?
				operstack.pop();
				operstack.push(ternaryOp);
			}
		}
		if (current.type() == OPERATOR) {
			if (current.value() == "++" || current.value() == "--") {
				// this is kindof problematic, coz `a[i]++` is a valid expression.
				// support for these operators will be added later. as of now ignore them.
				this->sendError(Error("ns", current.value(), current.lineNumber()));
			}
			else {
				Token oper, tval;
				while (!operstack.empty()) {
					tval = operstack.top();
					if (tval.value() == "(" || tval.value() == "[") break;
					if (Operations::comparePriority(current, tval) >= 0) {
						operstack.pop(oper);
						output.push(oper);
					}
					else break;
				}
				operstack.push(current);
			}
		}
		if (current.type() == LITERAL) {
			output.push(current);
		}
		prevtok = current;
	}

	// finally, after parsing the infix buffer, append the pre and post RPN.
	RPN finalOutput;
	while (preChange.pop(current)) finalOutput.push(current);
	while (output.pop(current)) finalOutput.push(current);
	while (postChange.pop(current)) finalOutput.push(current);

	return finalOutput;
}

Token Parser::toArgsToken(__SIZETYPE num) {
	String s = "@args|";
	s += integerToString(num);
	return Token(s, DIRECTIVE, FUNCTION);
}
/*** END implementation: class Parser ***/
#endif