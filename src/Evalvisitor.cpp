#include "Evalvisitor.h"
#include "Python3Lexer.h"
#include <stdexcept>
#include <cctype>
#include <regex>

// BigInt Implementation
void BigInt::normalize() {
    // Remove leading zeros
    size_t pos = value.find_first_not_of('0');
    if (pos == std::string::npos) {
        value = "0";
        negative = false;
    } else {
        value = value.substr(pos);
    }
    if (value == "0") negative = false;
}

BigInt::BigInt() : value("0"), negative(false) {}

BigInt::BigInt(const std::string& s) {
    if (s.empty() || s == "-") {
        value = "0";
        negative = false;
        return;
    }
    
    size_t start = 0;
    negative = false;
    if (s[0] == '-') {
        negative = true;
        start = 1;
    } else if (s[0] == '+') {
        start = 1;
    }
    
    value = s.substr(start);
    normalize();
}

BigInt::BigInt(long long n) {
    if (n < 0) {
        negative = true;
        n = -n;
    } else {
        negative = false;
    }
    value = std::to_string(n);
    normalize();
}

BigInt::BigInt(int n) : BigInt((long long)n) {}

BigInt::BigInt(const BigInt& other) : value(other.value), negative(other.negative) {}

std::string BigInt::addStrings(const std::string& a, const std::string& b) {
    std::string result;
    int carry = 0;
    int i = a.length() - 1;
    int j = b.length() - 1;
    
    while (i >= 0 || j >= 0 || carry) {
        int sum = carry;
        if (i >= 0) sum += a[i--] - '0';
        if (j >= 0) sum += b[j--] - '0';
        result = char('0' + sum % 10) + result;
        carry = sum / 10;
    }
    
    return result;
}

std::string BigInt::subtractStrings(const std::string& a, const std::string& b) {
    // Assumes a >= b
    std::string result;
    int borrow = 0;
    int i = a.length() - 1;
    int j = b.length() - 1;
    
    while (i >= 0) {
        int diff = (a[i--] - '0') - borrow;
        if (j >= 0) diff -= (b[j--] - '0');
        
        if (diff < 0) {
            diff += 10;
            borrow = 1;
        } else {
            borrow = 0;
        }
        result = char('0' + diff) + result;
    }
    
    // Remove leading zeros
    size_t pos = result.find_first_not_of('0');
    if (pos == std::string::npos) return "0";
    return result.substr(pos);
}

std::string BigInt::multiplyStrings(const std::string& a, const std::string& b) {
    int n = a.length(), m = b.length();
    std::vector<int> result(n + m, 0);
    
    for (int i = n - 1; i >= 0; i--) {
        for (int j = m - 1; j >= 0; j--) {
            int mul = (a[i] - '0') * (b[j] - '0');
            int p1 = i + j, p2 = i + j + 1;
            int sum = mul + result[p2];
            
            result[p2] = sum % 10;
            result[p1] += sum / 10;
        }
    }
    
    std::string str;
    for (int i : result) {
        if (!(str.empty() && i == 0)) {
            str += char('0' + i);
        }
    }
    
    return str.empty() ? "0" : str;
}

int BigInt::compareStrings(const std::string& a, const std::string& b) {
    if (a.length() != b.length()) {
        return a.length() > b.length() ? 1 : -1;
    }
    return a.compare(b);
}

std::pair<std::string, std::string> BigInt::divideStrings(const std::string& a, const std::string& b) {
    if (b == "0") throw std::runtime_error("Division by zero");
    
    std::string quotient = "0";
    std::string remainder = "0";
    
    for (char digit : a) {
        remainder = remainder == "0" ? std::string(1, digit) : remainder + digit;
        
        // Remove leading zeros from remainder
        size_t pos = remainder.find_first_not_of('0');
        if (pos != std::string::npos) {
            remainder = remainder.substr(pos);
        } else {
            remainder = "0";
        }
        
        int count = 0;
        while (compareStrings(remainder, b) >= 0) {
            remainder = subtractStrings(remainder, b);
            count++;
        }
        quotient = quotient == "0" ? std::to_string(count) : quotient + std::to_string(count);
    }
    
    if (quotient.empty()) quotient = "0";
    size_t pos = quotient.find_first_not_of('0');
    if (pos != std::string::npos) {
        quotient = quotient.substr(pos);
    } else {
        quotient = "0";
    }
    
    return {quotient, remainder};
}

BigInt BigInt::operator+(const BigInt& other) const {
    BigInt result;
    
    if (negative == other.negative) {
        result.value = addStrings(value, other.value);
        result.negative = negative;
    } else {
        int cmp = compareStrings(value, other.value);
        if (cmp > 0) {
            result.value = subtractStrings(value, other.value);
            result.negative = negative;
        } else if (cmp < 0) {
            result.value = subtractStrings(other.value, value);
            result.negative = other.negative;
        } else {
            result.value = "0";
            result.negative = false;
        }
    }
    
    result.normalize();
    return result;
}

BigInt BigInt::operator-(const BigInt& other) const {
    BigInt neg_other = other;
    neg_other.negative = !neg_other.negative;
    return *this + neg_other;
}

BigInt BigInt::operator*(const BigInt& other) const {
    BigInt result;
    result.value = multiplyStrings(value, other.value);
    result.negative = (negative != other.negative) && (result.value != "0");
    result.normalize();
    return result;
}

BigInt BigInt::operator/(const BigInt& other) const {
    if (other.isZero()) throw std::runtime_error("Division by zero");
    
    // Floor division
    auto [quot, rem] = divideStrings(value, other.value);
    BigInt result(quot);
    
    // Python floor division behavior
    // For different signs with non-zero remainder, we need -(q+1) instead of q
    if (negative != other.negative) {
        if (rem != "0") {
            result = result + BigInt(1);
        }
        result.negative = !result.isZero();
    } else {
        result.negative = false;
    }
    
    result.normalize();
    return result;
}

BigInt BigInt::operator%(const BigInt& other) const {
    if (other.isZero()) throw std::runtime_error("Division by zero");
    
    // a % b = a - (a // b) * b
    BigInt quotient = *this / other;
    BigInt result = *this - (quotient * other);
    result.normalize();
    return result;
}

BigInt BigInt::operator-() const {
    BigInt result = *this;
    if (!isZero()) {
        result.negative = !result.negative;
    }
    return result;
}

bool BigInt::operator<(const BigInt& other) const {
    if (negative != other.negative) return negative;
    if (negative) return compareStrings(value, other.value) > 0;
    return compareStrings(value, other.value) < 0;
}

bool BigInt::operator>(const BigInt& other) const {
    return other < *this;
}

bool BigInt::operator<=(const BigInt& other) const {
    return !(*this > other);
}

bool BigInt::operator>=(const BigInt& other) const {
    return !(*this < other);
}

bool BigInt::operator==(const BigInt& other) const {
    return negative == other.negative && value == other.value;
}

bool BigInt::operator!=(const BigInt& other) const {
    return !(*this == other);
}

std::string BigInt::toString() const {
    return (negative ? "-" : "") + value;
}

double BigInt::toDouble() const {
    double result = std::stod(value);
    return negative ? -result : result;
}

bool BigInt::isZero() const {
    return value == "0";
}

// Value Implementation
std::string Value::toString() const {
    switch (type) {
        case NONE: return "None";
        case BOOL: return boolVal ? "True" : "False";
        case INT: return intVal.toString();
        case FLOAT: {
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(6) << floatVal;
            return oss.str();
        }
        case STRING: return strVal;
        case TUPLE: {
            if (tupleVal.empty()) return "()";
            std::string result = "(";
            for (size_t i = 0; i < tupleVal.size(); i++) {
                if (i > 0) result += ", ";
                if (tupleVal[i].type == STRING) {
                    result += "'" + tupleVal[i].strVal + "'";
                } else {
                    result += tupleVal[i].toString();
                }
            }
            if (tupleVal.size() == 1) result += ",";
            result += ")";
            return result;
        }
        default: return "";
    }
}

bool Value::toBool() const {
    switch (type) {
        case NONE: return false;
        case BOOL: return boolVal;
        case INT: return !intVal.isZero();
        case FLOAT: return floatVal != 0.0;
        case STRING: return !strVal.empty();
        case TUPLE: return !tupleVal.empty();
        default: return false;
    }
}

double Value::toFloat() const {
    switch (type) {
        case BOOL: return boolVal ? 1.0 : 0.0;
        case INT: return intVal.toDouble();
        case FLOAT: return floatVal;
        case STRING: return std::stod(strVal);
        default: return 0.0;
    }
}

BigInt Value::toInt() const {
    switch (type) {
        case BOOL: return BigInt(boolVal ? 1 : 0);
        case INT: return intVal;
        case FLOAT: return BigInt((long long)floatVal);
        case STRING: return BigInt(strVal);
        default: return BigInt(0);
    }
}

Value Value::operator+(const Value& other) const {
    if (type == STRING || other.type == STRING) {
        return Value(toString() + other.toString());
    }
    if (type == FLOAT || other.type == FLOAT) {
        return Value(toFloat() + other.toFloat());
    }
    if (type == INT && other.type == INT) {
        return Value(intVal + other.intVal);
    }
    return Value();
}

Value Value::operator-(const Value& other) const {
    if (type == FLOAT || other.type == FLOAT) {
        return Value(toFloat() - other.toFloat());
    }
    if (type == INT && other.type == INT) {
        return Value(intVal - other.intVal);
    }
    return Value();
}

Value Value::operator*(const Value& other) const {
    if (type == STRING && other.type == INT) {
        std::string result;
        BigInt count = other.intVal;
        if (count.isNegative() || count.isZero()) return Value("");
        
        // For large counts, be careful
        long long n = count.toDouble();
        for (long long i = 0; i < n; i++) {
            result += strVal;
        }
        return Value(result);
    }
    if (type == INT && other.type == STRING) {
        return other * (*this);
    }
    if (type == FLOAT || other.type == FLOAT) {
        return Value(toFloat() * other.toFloat());
    }
    if (type == INT && other.type == INT) {
        return Value(intVal * other.intVal);
    }
    return Value();
}

Value Value::operator/(const Value& other) const {
    // Always float division
    return Value(toFloat() / other.toFloat());
}

Value Value::floordiv(const Value& other) const {
    if (type == INT && other.type == INT) {
        return Value(intVal / other.intVal);
    }
    // For floats, use floor
    double result = std::floor(toFloat() / other.toFloat());
    return Value(BigInt((long long)result));
}

Value Value::operator%(const Value& other) const {
    if (type == INT && other.type == INT) {
        return Value(intVal % other.intVal);
    }
    // For floats, use fmod with Python semantics
    double a = toFloat();
    double b = other.toFloat();
    double result = std::fmod(a, b);
    if ((result < 0 && b > 0) || (result > 0 && b < 0)) {
        result += b;
    }
    return Value(result);
}

Value Value::operator-() const {
    if (type == INT) return Value(-intVal);
    if (type == FLOAT) return Value(-floatVal);
    return Value();
}

bool Value::operator<(const Value& other) const {
    if (type == STRING && other.type == STRING) {
        return strVal < other.strVal;
    }
    if (type == FLOAT || other.type == FLOAT) {
        return toFloat() < other.toFloat();
    }
    if (type == INT && other.type == INT) {
        return intVal < other.intVal;
    }
    return false;
}

bool Value::operator>(const Value& other) const {
    return other < *this;
}

bool Value::operator<=(const Value& other) const {
    return !(*this > other);
}

bool Value::operator>=(const Value& other) const {
    return !(*this < other);
}

bool Value::operator==(const Value& other) const {
    // Type conversions for ==
    if (type == other.type) {
        switch (type) {
            case NONE: return true;
            case BOOL: return boolVal == other.boolVal;
            case INT: return intVal == other.intVal;
            case FLOAT: return floatVal == other.floatVal;
            case STRING: return strVal == other.strVal;
            default: return false;
        }
    }
    
    // Cross-type comparisons (except STRING)
    if (type == STRING || other.type == STRING) return false;
    
    if ((type == INT || type == FLOAT || type == BOOL) && 
        (other.type == INT || other.type == FLOAT || other.type == BOOL)) {
        if (type == FLOAT || other.type == FLOAT) {
            return toFloat() == other.toFloat();
        }
        return toInt() == other.toInt();
    }
    
    return false;
}

bool Value::operator!=(const Value& other) const {
    return !(*this == other);
}

// EvalVisitor Implementation
EvalVisitor::EvalVisitor() {}

Value EvalVisitor::getValue(const std::string& name) {
    // Check local scopes first (from innermost to outermost)
    for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
        if (it->find(name) != it->end()) {
            return (*it)[name];
        }
    }
    // Check global scope
    if (globalScope.find(name) != globalScope.end()) {
        return globalScope[name];
    }
    return Value(); // None
}

void EvalVisitor::setValue(const std::string& name, const Value& val) {
    if (!scopeStack.empty()) {
        // Check if variable exists in local scopes first
        for (auto it = scopeStack.rbegin(); it != scopeStack.rend(); ++it) {
            if (it->find(name) != it->end()) {
                (*it)[name] = val;
                return;
            }
        }
        // Check if it exists in global scope
        if (globalScope.find(name) != globalScope.end()) {
            globalScope[name] = val;
            return;
        }
        // Not found anywhere, set in current scope
        scopeStack.back()[name] = val;
    } else {
        globalScope[name] = val;
    }
}

void EvalVisitor::enterScope() {
    scopeStack.push_back(std::map<std::string, Value>());
}

void EvalVisitor::exitScope() {
    if (!scopeStack.empty()) {
        scopeStack.pop_back();
    }
}

std::string EvalVisitor::evaluateFString(const std::string& fstr) {
    std::string result;
    size_t i = 0;
    
    while (i < fstr.length()) {
        if (fstr[i] == '{') {
            if (i + 1 < fstr.length() && fstr[i + 1] == '{') {
                result += '{';
                i += 2;
            } else {
                // Find matching }
                int depth = 1;
                size_t j = i + 1;
                while (j < fstr.length() && depth > 0) {
                    if (fstr[j] == '{') depth++;
                    else if (fstr[j] == '}') depth--;
                    if (depth > 0) j++;
                }
                
                std::string expr = fstr.substr(i + 1, j - i - 1);
                
                // Parse and evaluate the expression
                antlr4::ANTLRInputStream input(expr);
                Python3Lexer lexer(&input);
                antlr4::CommonTokenStream tokens(&lexer);
                Python3Parser parser(&tokens);
                auto testCtx = parser.test();
                
                Value val = std::any_cast<Value>(visit(testCtx));
                
                // Format the value
                if (val.type == Value::BOOL) {
                    result += val.boolVal ? "True" : "False";
                } else if (val.type == Value::FLOAT) {
                    std::ostringstream oss;
                    oss << std::fixed << std::setprecision(6) << val.floatVal;
                    result += oss.str();
                } else {
                    result += val.toString();
                }
                
                i = j + 1;
            }
        } else if (fstr[i] == '}') {
            if (i + 1 < fstr.length() && fstr[i + 1] == '}') {
                result += '}';
                i += 2;
            } else {
                i++;
            }
        } else {
            result += fstr[i];
            i++;
        }
    }
    
    return result;
}

std::any EvalVisitor::visitFile_input(Python3Parser::File_inputContext *ctx) {
    return visitChildren(ctx);
}

std::any EvalVisitor::visitFuncdef(Python3Parser::FuncdefContext *ctx) {
    std::string funcName = ctx->NAME()->getText();
    
    FunctionDef func;
    func.body = ctx->suite();
    func.globalScope = &globalScope;
    
    // Parse parameters
    if (ctx->parameters() && ctx->parameters()->typedargslist()) {
        auto paramList = ctx->parameters()->typedargslist();
        
        auto tfpdefs = paramList->tfpdef();
        auto tests = paramList->test();
        
        int defaultStart = tfpdefs.size() - tests.size();
        
        for (size_t i = 0; i < tfpdefs.size(); i++) {
            func.params.push_back(tfpdefs[i]->NAME()->getText());
            
            if ((int)i >= defaultStart) {
                int defaultIdx = i - defaultStart;
                Value defaultVal = std::any_cast<Value>(visit(tests[defaultIdx]));
                func.defaults.push_back(defaultVal);
            }
        }
    }
    
    functions[funcName] = func;
    return Value();
}

std::any EvalVisitor::visitParameters(Python3Parser::ParametersContext *ctx) {
    return visitChildren(ctx);
}

std::any EvalVisitor::visitTypedargslist(Python3Parser::TypedargslistContext *ctx) {
    return visitChildren(ctx);
}

std::any EvalVisitor::visitStmt(Python3Parser::StmtContext *ctx) {
    return visitChildren(ctx);
}

std::any EvalVisitor::visitSimple_stmt(Python3Parser::Simple_stmtContext *ctx) {
    return visitChildren(ctx);
}

std::any EvalVisitor::visitSmall_stmt(Python3Parser::Small_stmtContext *ctx) {
    return visitChildren(ctx);
}

std::any EvalVisitor::visitExpr_stmt(Python3Parser::Expr_stmtContext *ctx) {
    auto testlists = ctx->testlist();
    
    if (testlists.size() == 1) {
        // Just an expression
        return visit(testlists[0]);
    }
    
    // Assignment
    if (ctx->augassign()) {
        // Augmented assignment
        std::string op = ctx->augassign()->getText();
        Value rightVal = std::any_cast<Value>(visit(testlists[1]));
        
        // Get variable name(s) from left side
        auto tests = testlists[0]->test();
        for (auto test : tests) {
            if (test->or_test() && test->or_test()->and_test().size() == 1) {
                auto andTest = test->or_test()->and_test(0);
                if (andTest->not_test().size() == 1) {
                    auto notTest = andTest->not_test(0);
                    if (notTest->comparison() && notTest->comparison()->arith_expr().size() == 1) {
                        auto arithExpr = notTest->comparison()->arith_expr(0);
                        if (arithExpr->term().size() == 1) {
                            auto term = arithExpr->term(0);
                            if (term->factor().size() == 1) {
                                auto factor = term->factor(0);
                                if (factor->atom_expr()) {
                                    auto atomExpr = factor->atom_expr();
                                    if (atomExpr->atom() && atomExpr->atom()->NAME()) {
                                        std::string varName = atomExpr->atom()->NAME()->getText();
                                        Value leftVal = getValue(varName);
                                        
                                        Value newVal;
                                        if (op == "+=") newVal = leftVal + rightVal;
                                        else if (op == "-=") newVal = leftVal - rightVal;
                                        else if (op == "*=") newVal = leftVal * rightVal;
                                        else if (op == "/=") newVal = leftVal / rightVal;
                                        else if (op == "//=") newVal = leftVal.floordiv(rightVal);
                                        else if (op == "%=") newVal = leftVal % rightVal;
                                        
                                        setValue(varName, newVal);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    } else {
        // Regular assignment: a = b = c = value
        Value value = std::any_cast<Value>(visit(testlists.back()));
        
        // Assign to all targets
        for (size_t i = 0; i < testlists.size() - 1; i++) {
            auto tests = testlists[i]->test();
            
            if (tests.size() == 1) {
                // Single assignment
                std::string varName = tests[0]->getText();
                // Extract actual variable name
                if (tests[0]->or_test() && tests[0]->or_test()->and_test().size() == 1) {
                    auto andTest = tests[0]->or_test()->and_test(0);
                    if (andTest->not_test().size() == 1) {
                        auto notTest = andTest->not_test(0);
                        if (notTest->comparison() && notTest->comparison()->arith_expr().size() == 1) {
                            auto arithExpr = notTest->comparison()->arith_expr(0);
                            if (arithExpr->term().size() == 1) {
                                auto term = arithExpr->term(0);
                                if (term->factor().size() == 1) {
                                    auto factor = term->factor(0);
                                    if (factor->atom_expr()) {
                                        auto atomExpr = factor->atom_expr();
                                        if (atomExpr->atom() && atomExpr->atom()->NAME()) {
                                            varName = atomExpr->atom()->NAME()->getText();
                                            setValue(varName, value);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            } else {
                // Multiple assignment: a, b = 1, 2
                if (value.type == Value::TUPLE) {
                    for (size_t j = 0; j < tests.size() && j < value.tupleVal.size(); j++) {
                        std::string varName = tests[j]->getText();
                        if (tests[j]->or_test() && tests[j]->or_test()->and_test().size() == 1) {
                            auto andTest = tests[j]->or_test()->and_test(0);
                            if (andTest->not_test().size() == 1) {
                                auto notTest = andTest->not_test(0);
                                if (notTest->comparison() && notTest->comparison()->arith_expr().size() == 1) {
                                    auto arithExpr = notTest->comparison()->arith_expr(0);
                                    if (arithExpr->term().size() == 1) {
                                        auto term = arithExpr->term(0);
                                        if (term->factor().size() == 1) {
                                            auto factor = term->factor(0);
                                            if (factor->atom_expr()) {
                                                auto atomExpr = factor->atom_expr();
                                                if (atomExpr->atom() && atomExpr->atom()->NAME()) {
                                                    varName = atomExpr->atom()->NAME()->getText();
                                                    setValue(varName, value.tupleVal[j]);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    return Value();
}

std::any EvalVisitor::visitAugassign(Python3Parser::AugassignContext *ctx) {
    return visitChildren(ctx);
}

std::any EvalVisitor::visitFlow_stmt(Python3Parser::Flow_stmtContext *ctx) {
    return visitChildren(ctx);
}

std::any EvalVisitor::visitBreak_stmt(Python3Parser::Break_stmtContext *ctx) {
    throw BreakException();
}

std::any EvalVisitor::visitContinue_stmt(Python3Parser::Continue_stmtContext *ctx) {
    throw ContinueException();
}

std::any EvalVisitor::visitReturn_stmt(Python3Parser::Return_stmtContext *ctx) {
    if (ctx->testlist()) {
        Value val = std::any_cast<Value>(visit(ctx->testlist()));
        throw ReturnException(val);
    }
    throw ReturnException(Value());
}

std::any EvalVisitor::visitCompound_stmt(Python3Parser::Compound_stmtContext *ctx) {
    return visitChildren(ctx);
}

std::any EvalVisitor::visitIf_stmt(Python3Parser::If_stmtContext *ctx) {
    auto tests = ctx->test();
    auto suites = ctx->suite();
    
    for (size_t i = 0; i < tests.size(); i++) {
        Value condition = std::any_cast<Value>(visit(tests[i]));
        if (condition.toBool()) {
            visit(suites[i]);
            return Value();
        }
    }
    
    // else clause
    if (suites.size() > tests.size()) {
        visit(suites.back());
    }
    
    return Value();
}

std::any EvalVisitor::visitWhile_stmt(Python3Parser::While_stmtContext *ctx) {
    while (true) {
        Value condition = std::any_cast<Value>(visit(ctx->test()));
        if (!condition.toBool()) break;
        
        try {
            visit(ctx->suite());
        } catch (BreakException&) {
            break;
        } catch (ContinueException&) {
            continue;
        }
    }
    
    return Value();
}

std::any EvalVisitor::visitSuite(Python3Parser::SuiteContext *ctx) {
    return visitChildren(ctx);
}

std::any EvalVisitor::visitTest(Python3Parser::TestContext *ctx) {
    return visit(ctx->or_test());
}

std::any EvalVisitor::visitOr_test(Python3Parser::Or_testContext *ctx) {
    auto andTests = ctx->and_test();
    Value result = std::any_cast<Value>(visit(andTests[0]));
    
    for (size_t i = 1; i < andTests.size(); i++) {
        if (result.toBool()) {
            return result; // Short-circuit
        }
        result = std::any_cast<Value>(visit(andTests[i]));
    }
    
    return result;
}

std::any EvalVisitor::visitAnd_test(Python3Parser::And_testContext *ctx) {
    auto notTests = ctx->not_test();
    Value result = std::any_cast<Value>(visit(notTests[0]));
    
    for (size_t i = 1; i < notTests.size(); i++) {
        if (!result.toBool()) {
            return result; // Short-circuit
        }
        result = std::any_cast<Value>(visit(notTests[i]));
    }
    
    return result;
}

std::any EvalVisitor::visitNot_test(Python3Parser::Not_testContext *ctx) {
    if (ctx->NOT()) {
        Value val = std::any_cast<Value>(visit(ctx->not_test()));
        return Value(!val.toBool());
    }
    return visit(ctx->comparison());
}

std::any EvalVisitor::visitComparison(Python3Parser::ComparisonContext *ctx) {
    auto arithExprs = ctx->arith_expr();
    
    if (arithExprs.size() == 1) {
        return visit(arithExprs[0]);
    }
    
    // Chained comparisons
    std::vector<Value> values;
    for (auto expr : arithExprs) {
        values.push_back(std::any_cast<Value>(visit(expr)));
    }
    
    auto compOps = ctx->comp_op();
    for (size_t i = 0; i < compOps.size(); i++) {
        std::string op = compOps[i]->getText();
        bool result;
        
        if (op == "<") result = values[i] < values[i + 1];
        else if (op == ">") result = values[i] > values[i + 1];
        else if (op == "<=") result = values[i] <= values[i + 1];
        else if (op == ">=") result = values[i] >= values[i + 1];
        else if (op == "==") result = values[i] == values[i + 1];
        else if (op == "!=") result = values[i] != values[i + 1];
        else result = false;
        
        if (!result) return Value(false);
    }
    
    return Value(true);
}

std::any EvalVisitor::visitArith_expr(Python3Parser::Arith_exprContext *ctx) {
    auto terms = ctx->term();
    Value result = std::any_cast<Value>(visit(terms[0]));
    
    for (size_t i = 1; i < terms.size(); i++) {
        Value right = std::any_cast<Value>(visit(terms[i]));
        std::string op = ctx->children[i * 2 - 1]->getText();
        
        if (op == "+") result = result + right;
        else if (op == "-") result = result - right;
    }
    
    return result;
}

std::any EvalVisitor::visitTerm(Python3Parser::TermContext *ctx) {
    auto factors = ctx->factor();
    Value result = std::any_cast<Value>(visit(factors[0]));
    
    for (size_t i = 1; i < factors.size(); i++) {
        Value right = std::any_cast<Value>(visit(factors[i]));
        std::string op = ctx->children[i * 2 - 1]->getText();
        
        if (op == "*") result = result * right;
        else if (op == "/") result = result / right;
        else if (op == "//") result = result.floordiv(right);
        else if (op == "%") result = result % right;
    }
    
    return result;
}

std::any EvalVisitor::visitFactor(Python3Parser::FactorContext *ctx) {
    if (ctx->factor()) {
        Value val = std::any_cast<Value>(visit(ctx->factor()));
        std::string op = ctx->children[0]->getText();
        
        if (op == "-") return -val;
        else if (op == "+") return val;
    }
    
    return visit(ctx->atom_expr());
}

std::any EvalVisitor::visitAtom_expr(Python3Parser::Atom_exprContext *ctx) {
    Value result = std::any_cast<Value>(visit(ctx->atom()));
    
    // Handle trailer (function call)
    auto trailer = ctx->trailer();
    if (trailer) {
        if (trailer->arglist() || trailer->getText() == "()") {
            // Function call - get the function name from the atom
            std::string funcName = ctx->atom()->getText();
            
            // Built-in functions
            if (funcName == "print") {
                std::vector<Value> args;
                if (trailer->arglist()) {
                    auto arguments = trailer->arglist()->argument();
                    for (auto arg : arguments) {
                        args.push_back(std::any_cast<Value>(visit(arg)));
                    }
                }
                
                for (size_t i = 0; i < args.size(); i++) {
                    if (i > 0) std::cout << " ";
                    if (args[i].type == Value::STRING) {
                        std::cout << args[i].strVal;
                    } else if (args[i].type == Value::FLOAT) {
                        std::cout << std::fixed << std::setprecision(6) << args[i].floatVal;
                    } else {
                        std::cout << args[i].toString();
                    }
                }
                std::cout << std::endl;
                
                result = Value();
            } else if (funcName == "int") {
                Value arg = std::any_cast<Value>(visit(trailer->arglist()->argument(0)));
                result = Value(arg.toInt());
            } else if (funcName == "float") {
                Value arg = std::any_cast<Value>(visit(trailer->arglist()->argument(0)));
                result = Value(arg.toFloat());
            } else if (funcName == "str") {
                Value arg = std::any_cast<Value>(visit(trailer->arglist()->argument(0)));
                result = Value(arg.toString());
            } else if (funcName == "bool") {
                Value arg = std::any_cast<Value>(visit(trailer->arglist()->argument(0)));
                result = Value(arg.toBool());
            } else if (functions.find(funcName) != functions.end()) {
                // User-defined function
                FunctionDef& func = functions[funcName];
                
                // Prepare arguments
                std::map<std::string, Value> args;
                
                if (trailer->arglist()) {
                    auto arguments = trailer->arglist()->argument();
                    size_t posArgCount = 0;
                    
                    for (auto arg : arguments) {
                        if (arg->test().size() == 2 && arg->children[1]->getText() == "=") {
                            // Keyword argument
                            std::string paramName = arg->test(0)->getText();
                            Value val = std::any_cast<Value>(visit(arg->test(1)));
                            args[paramName] = val;
                        } else {
                            // Positional argument
                            if (posArgCount < func.params.size()) {
                                Value val = std::any_cast<Value>(visit(arg));
                                args[func.params[posArgCount]] = val;
                                posArgCount++;
                            }
                        }
                    }
                    
                    // Fill in default values
                    size_t defaultStart = func.params.size() - func.defaults.size();
                    for (size_t i = 0; i < func.params.size(); i++) {
                        if (args.find(func.params[i]) == args.end()) {
                            if (i >= defaultStart) {
                                args[func.params[i]] = func.defaults[i - defaultStart];
                            }
                        }
                    }
                }
                
                // Create new scope and execute function
                enterScope();
                for (auto& p : args) {
                    setValue(p.first, p.second);
                }
                
                try {
                    visit(func.body);
                    result = Value(); // No return value
                } catch (ReturnException& e) {
                    result = e.value;
                }
                
                exitScope();
            }
        }
    }
    
    return result;
}

std::any EvalVisitor::visitTrailer(Python3Parser::TrailerContext *ctx) {
    return visitChildren(ctx);
}

std::any EvalVisitor::visitAtom(Python3Parser::AtomContext *ctx) {
    if (ctx->NONE()) {
        return Value();
    }
    
    if (ctx->TRUE()) {
        return Value(true);
    }
    
    if (ctx->FALSE()) {
        return Value(false);
    }
    
    if (ctx->NAME()) {
        std::string name = ctx->NAME()->getText();
        
        // Check if it's a function name (for function calls)
        if (functions.find(name) != functions.end()) {
            return Value(name); // Return function name as string
        }
        
        return getValue(name);
    }
    
    if (ctx->NUMBER()) {
        std::string num = ctx->NUMBER()->getText();
        if (num.find('.') != std::string::npos) {
            return Value(std::stod(num));
        } else {
            return Value(BigInt(num));
        }
    }
    
    if (!ctx->STRING().empty()) {
        std::string result;
        bool isFString = false;
        
        for (auto str : ctx->STRING()) {
            std::string s = str->getText();
            
            // Check if it's an f-string
            if (s[0] == 'f' || s[0] == 'F') {
                isFString = true;
                s = s.substr(1);
            }
            
            // Remove quotes
            s = s.substr(1, s.length() - 2);
            
            if (isFString) {
                result += evaluateFString(s);
            } else {
                result += s;
            }
        }
        
        return Value(result);
    }
    
    if (ctx->test()) {
        return visit(ctx->test());
    }
    
    if (ctx->format_string()) {
        return visit(ctx->format_string());
    }
    
    return Value();
}

std::any EvalVisitor::visitFormat_string(Python3Parser::Format_stringContext *ctx) {
    std::string result;
    
    // Get all children and process them
    for (size_t i = 0; i < ctx->children.size(); i++) {
        auto child = ctx->children[i];
        std::string text = child->getText();
        
        if (text == "f\"" || text == "f'" || text == "\"" || text == "'" || 
            text == "F\"" || text == "F'") {
            // Skip quotation marks
            continue;
        } else if (text == "{") {
            // Expression follows
            i++; // Move to testlist
            if (i < ctx->children.size()) {
                auto testlistCtx = dynamic_cast<Python3Parser::TestlistContext*>(ctx->children[i]);
                if (testlistCtx) {
                    Value val = std::any_cast<Value>(visit(testlistCtx));
                    
                    // Format the value
                    if (val.type == Value::BOOL) {
                        result += val.boolVal ? "True" : "False";
                    } else if (val.type == Value::FLOAT) {
                        std::ostringstream oss;
                        oss << std::fixed << std::setprecision(6) << val.floatVal;
                        result += oss.str();
                    } else {
                        result += val.toString();
                    }
                }
                i++; // Skip the closing }
            }
        } else if (auto terminal = dynamic_cast<antlr4::tree::TerminalNode*>(child)) {
            // It's a FORMAT_STRING_LITERAL
            std::string literal = terminal->getText();
            // Handle escape sequences
            for (size_t j = 0; j < literal.length(); j++) {
                if (literal[j] == '{' && j + 1 < literal.length() && literal[j + 1] == '{') {
                    result += '{';
                    j++;
                } else if (literal[j] == '}' && j + 1 < literal.length() && literal[j + 1] == '}') {
                    result += '}';
                    j++;
                } else {
                    result += literal[j];
                }
            }
        }
    }
    
    return Value(result);
}

std::any EvalVisitor::visitTestlist(Python3Parser::TestlistContext *ctx) {
    auto tests = ctx->test();
    
    if (tests.size() == 1) {
        return visit(tests[0]);
    }
    
    // Multiple values - return as tuple
    std::vector<Value> values;
    for (auto test : tests) {
        values.push_back(std::any_cast<Value>(visit(test)));
    }
    
    return Value(values);
}

std::any EvalVisitor::visitArglist(Python3Parser::ArglistContext *ctx) {
    return visitChildren(ctx);
}

std::any EvalVisitor::visitArgument(Python3Parser::ArgumentContext *ctx) {
    if (ctx->test().size() == 1) {
        return visit(ctx->test(0));
    }
    return visitChildren(ctx);
}

std::any EvalVisitor::visitComp_op(Python3Parser::Comp_opContext *ctx) {
    return visitChildren(ctx);
}
