#ifdef COMPONENT_EVALUATOR_H

Evaluator::Evaluator(Parser *pr, String args) {
	this->parser = pr;
	this->failed = false;
}

bool Evaluator::runProgram() {
	// initialise the HEAP
	this->variables = this->parser->variables;
	
	RPN primarySource = this->parser->output;
	Vector<Token> rets;
	
	this->evaluateRPN(primarySource, this->variables, &rets);
	if (this->failed) {
		return this->showErrors(cerr);
	}
	return true;
}

/*** interface ***/
bool Evaluator::sendError(Error e) {
	this->errors.pushback(e);
	this->failed = true;
	return true;
}
bool Evaluator::sendError(String cd, String msg, bufferIndex ln, int s) {
	return this->sendError(Error(cd, msg, ln, s));
}

bool Evaluator::showErrors(ostream& out, bool clearAfterDisplay) {
	if (this->errors.size() > 0) {
		for (__SIZETYPE i = 0; i < errors.size(); i++) {
			out << errors[i].message() << endl;
		}
		if (clearAfterDisplay) errors.clear();
		return true;
	}
	return false;
}

/*** Variable Caches and management ***/
Variable& Evaluator::getCachedVariable(String id) {
	if (id.substr(0, 2) == "#c") {
		long index = id.substr(2).toNumber();
		if (index < 0 || index > this->cache.size()) return nullVariableRef;
		return *(this->cache[index]);
	}
	return nullVariableRef;
}

String Evaluator::cacheVariable(Variable v) {
	Variable *ref = new Variable();
	ref->setValue(v);
	return this->cacheVariableRef(ref);
}
String Evaluator::cacheVariableRef(Variable *ref) {
	this->cache.pushback(ref);
	long index = this->cache.size() - 1;
	String hash = "#c";
	hash += integerToString(index);
	return hash;
}

Variable& Evaluator::getVariable(String id, VariableScope& scope, bool searchCache) {
	if (scope.exists(id)) {
		return scope.resolve(id);
	}
	if (searchCache) {
		Variable& v = this->getCachedVariable(id);
		return v;
	}
	return nullVariableRef;
}

Vector<Variable>& Evaluator::getGlobals() {
	return this->variables.getBaseVariables();
}

Token Evaluator::evaluateRPN(RPN source, VariableScope& scope, Vector<Token>* statementValues) {
	if (this->failed) return Token();
	Token current;
	Stack<Token> valuestack;
	// static 
	Vector<String> assignmentOp(strsplit("= += -= *= /= %=", ' '));

	while (source.pop(current)) {
		if (current.type() == LITERAL || current.subtype() == VARIABLE) valuestack.push(current);
		else if (current.value() == ";") {
			Token t;
			// if (valuestack.pop(t) && statementValues != NULL) statementValues->pushback(t);
		}
		else if (current.type() == OPERATOR) {
			if (current.value() == "[]") {
				Token a, b;
				valuestack.pop(b);
				valuestack.pop(a);
				if (a.type() == LITERAL) {
					if (a.subtype() == STRING && b.value().isInteger()) {
						__SIZETYPE ind = b.value().toInteger();
						String str = Lexer::tokenToString(a);
						if (ind < 0 || ind >= str.length()) this->sendError(Error("r")); // invalid index
						else {
							str = Lexer::stringToLiteral(str.substr(ind, 1));
							valuestack.push(Lexer::toToken(str));
						}
					} else {
						this->sendError(Error("r", a.value() + " and " + b.value())); // invalid types for []
					}
				}
				else {
					
				}
			}
			else if (current.subtype() == UNARYOP) {
				Token a;
				valuestack.pop(a);
				valuestack.push(Operations::unaryOperator(current.value(), a));
			}
			else if (current.subtype() == BINARYOP) {
				// check if it is assignment operator:
				String oper = current.value();
				
				bool isAssign = false;
				
				if (assignmentOp.indexOf(current.value()) >= 0) {
					oper = current.value().substr(0, 1);
					isAssign = true;
				}
				
				Token a, b, va, vb;
				valuestack.pop(b);
				valuestack.pop(a);
				va = a;
				vb = b;
				if (a.subtype() == VARIABLE) {
					Variable& v1 = this->getVariable(a.value(), scope, true);
					if (v1.id() == ";") 
				}
				if (b.subtype() == VARIABLE) {
					Variable& v2 = this->getVariable(a.value(), scope, true);
				}

				Token res = Operations::binaryOperator(oper, va, vb);

				if (isAssign) {
					if (oper == "=") { // a = b
						if (scope.exists(a.value())) scope.resolve(a.value()).setValue(Variable(b));
						res = a;
					}
					else { // a = res
						if (scope.exists(a.value())) scope.resolve(a.value()).setValue(Variable(res));
					}
				}
				valuestack.push(res);
			}
		}
		else if (current.type() == KEYWORD) {

			if (current.value() == "if") {
				Token hashtok;
				source.pop(hashtok);
				HashedData hd = this->parser->getHashedData(hashtok.value());
				HashedData::csIf ifSet = hd.getIf();

				Token val = evaluateRPN(ifSet.ifCondition, scope);
				val = Operations::typecastToken(val, BOOLEAN);

				if (val.value() == "true") {
					scope.stackVariables(ifSet.ifVariables);
					RPN r = ifSet.ifStatements;
					while (r.pop(val)) DEBUG(val.value())
					evaluateRPN(ifSet.ifStatements, scope);
					scope.popVariables();
				}
				else {
					scope.stackVariables(ifSet.elseVariables);
					evaluateRPN(ifSet.elseStatements, scope);
					scope.popVariables();
				}
			}
			
		}
		else if (current.type() == IDENTIFIER) {
			if (current.subtype() == FUNCTION) {
				Token argtok, inv, tmp;
				source.pop(argtok); source.pop(inv);
				
				long numOfArgs = argtok.value().substr(6).toInteger(); // argtok.value(): @args|<int>
				Vector<Token> args;
				while (numOfArgs--) {
					if (valuestack.pop(tmp)) args.pushfront(tmp);
				}
				
				if (inv.value() == ".") { // dereference the method.
				
				}
				else { // global function:
				PRINT("global func")
					if (InbuiltFunctionList.indexOf(current.value()) >= 0) {
						Token res;
						if (current.value() == "print") {
							for (__SIZETYPE i = 0; i < args.size(); i++) {
								if (args[i].type() == LITERAL) InbuiltFunctions::write(args[i]);
								else if (args[i].subtype() == VARIABLE) {
									Variable v;
									if (args[i].value()[0] == '#') {
										v = this->getCachedVariable(args[i].value());
										InbuiltFunctions::write(v.value());
									}
									else if (args[i].type() == IDENTIFIER && scope.exists(args[i].value())) {
										v = scope.resolve(args[i].value());
										InbuiltFunctions::write(v.value());
									}
								}
							}
						}
					}
					else {
						Function func = this->parser->getFunction(current.value());
						if (!!func.id()) {
							Vector<Variable> argVars;
							for (__SIZETYPE i = 0; i < args.size(); i++) {
								if (args[i].type() == LITERAL) argVars.pushback(Variable(args[i]));
								else if (args[i].subtype() == VARIABLE) {
									if (args[i].value()[0] == '#') 
										argVars.pushback(this->getCachedVariable(args[i].value()));
									else if (args[i].type() == IDENTIFIER && scope.exists(args[i].value()))
										argVars.pushback(scope.resolve(args[i].value()));
								}
							}
							tmp = func.evaluate(argVars, *this);
						} else {
							this->sendError("r", current.value()); // unable to find function.
						}
					}
				}
			}
			else if (current.subtype() == VARIABLE) valuestack.push(current);
		}
	}
	Token ret;
	if (!valuestack.pop(ret) && statementValues) {
		statementValues->popback(ret);
	}
	while (valuestack.pop(current) && statementValues) statementValues->pushback(current);
	return ret;
}

#endif // COMPONENT_EVALUATOR_H