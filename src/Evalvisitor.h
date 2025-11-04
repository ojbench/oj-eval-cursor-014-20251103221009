#pragma once
#ifndef PYTHON_INTERPRETER_EVALVISITOR_H
#define PYTHON_INTERPRETER_EVALVISITOR_H

#include "Python3ParserBaseVisitor.h"
#include <string>
#include <map>
#include <vector>
#include <memory>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>
#include <algorithm>

// BigInteger class for arbitrary precision arithmetic
class BigInt {
private:
    std::string value;
    bool negative;
    
    void normalize();
    static std::string addStrings(const std::string& a, const std::string& b);
    static std::string subtractStrings(const std::string& a, const std::string& b);
    static std::string multiplyStrings(const std::string& a, const std::string& b);
    static std::pair<std::string, std::string> divideStrings(const std::string& a, const std::string& b);
    static int compareStrings(const std::string& a, const std::string& b);
    
public:
    BigInt();
    BigInt(const std::string& s);
    BigInt(long long n);
    BigInt(int n);
    BigInt(const BigInt& other);
    
    BigInt operator+(const BigInt& other) const;
    BigInt operator-(const BigInt& other) const;
    BigInt operator*(const BigInt& other) const;
    BigInt operator/(const BigInt& other) const; // floor division
    BigInt operator%(const BigInt& other) const;
    BigInt operator-() const;
    
    bool operator<(const BigInt& other) const;
    bool operator>(const BigInt& other) const;
    bool operator<=(const BigInt& other) const;
    bool operator>=(const BigInt& other) const;
    bool operator==(const BigInt& other) const;
    bool operator!=(const BigInt& other) const;
    
    std::string toString() const;
    double toDouble() const;
    bool isZero() const;
    bool isNegative() const { return negative; }
};

// Value class to hold different Python types
class Value {
public:
    enum Type { NONE, BOOL, INT, FLOAT, STRING, TUPLE, FUNCTION };
    
    Type type;
    bool boolVal;
    BigInt intVal;
    double floatVal;
    std::string strVal;
    std::vector<Value> tupleVal;
    
    Value() : type(NONE) {}
    Value(bool b) : type(BOOL), boolVal(b) {}
    Value(const BigInt& i) : type(INT), intVal(i) {}
    Value(int i) : type(INT), intVal(i) {}
    Value(double f) : type(FLOAT), floatVal(f) {}
    Value(const std::string& s) : type(STRING), strVal(s) {}
    Value(const std::vector<Value>& t) : type(TUPLE), tupleVal(t) {}
    
    std::string toString() const;
    bool toBool() const;
    double toFloat() const;
    BigInt toInt() const;
    
    Value operator+(const Value& other) const;
    Value operator-(const Value& other) const;
    Value operator*(const Value& other) const;
    Value operator/(const Value& other) const;
    Value operator%(const Value& other) const;
    Value floordiv(const Value& other) const;
    Value operator-() const;
    
    bool operator<(const Value& other) const;
    bool operator>(const Value& other) const;
    bool operator<=(const Value& other) const;
    bool operator>=(const Value& other) const;
    bool operator==(const Value& other) const;
    bool operator!=(const Value& other) const;
};

// Exception classes for control flow
class ReturnException {
public:
    Value value;
    ReturnException(const Value& v) : value(v) {}
};

class BreakException {};
class ContinueException {};

// Function definition
struct FunctionDef {
    std::vector<std::string> params;
    std::vector<Value> defaults;
    Python3Parser::SuiteContext* body;
    std::map<std::string, Value>* globalScope;
};

class EvalVisitor : public Python3ParserBaseVisitor {
private:
    std::map<std::string, Value> globalScope;
    std::vector<std::map<std::string, Value>> scopeStack;
    std::map<std::string, FunctionDef> functions;
    
    Value getValue(const std::string& name);
    void setValue(const std::string& name, const Value& val);
    void enterScope();
    void exitScope();
    
    std::string evaluateFString(const std::string& fstr);
    
public:
    EvalVisitor();
    
    virtual std::any visitFile_input(Python3Parser::File_inputContext *ctx) override;
    virtual std::any visitFuncdef(Python3Parser::FuncdefContext *ctx) override;
    virtual std::any visitParameters(Python3Parser::ParametersContext *ctx) override;
    virtual std::any visitTypedargslist(Python3Parser::TypedargslistContext *ctx) override;
    virtual std::any visitStmt(Python3Parser::StmtContext *ctx) override;
    virtual std::any visitSimple_stmt(Python3Parser::Simple_stmtContext *ctx) override;
    virtual std::any visitSmall_stmt(Python3Parser::Small_stmtContext *ctx) override;
    virtual std::any visitExpr_stmt(Python3Parser::Expr_stmtContext *ctx) override;
    virtual std::any visitAugassign(Python3Parser::AugassignContext *ctx) override;
    virtual std::any visitFlow_stmt(Python3Parser::Flow_stmtContext *ctx) override;
    virtual std::any visitBreak_stmt(Python3Parser::Break_stmtContext *ctx) override;
    virtual std::any visitContinue_stmt(Python3Parser::Continue_stmtContext *ctx) override;
    virtual std::any visitReturn_stmt(Python3Parser::Return_stmtContext *ctx) override;
    virtual std::any visitCompound_stmt(Python3Parser::Compound_stmtContext *ctx) override;
    virtual std::any visitIf_stmt(Python3Parser::If_stmtContext *ctx) override;
    virtual std::any visitWhile_stmt(Python3Parser::While_stmtContext *ctx) override;
    virtual std::any visitSuite(Python3Parser::SuiteContext *ctx) override;
    virtual std::any visitTest(Python3Parser::TestContext *ctx) override;
    virtual std::any visitOr_test(Python3Parser::Or_testContext *ctx) override;
    virtual std::any visitAnd_test(Python3Parser::And_testContext *ctx) override;
    virtual std::any visitNot_test(Python3Parser::Not_testContext *ctx) override;
    virtual std::any visitComparison(Python3Parser::ComparisonContext *ctx) override;
    virtual std::any visitArith_expr(Python3Parser::Arith_exprContext *ctx) override;
    virtual std::any visitTerm(Python3Parser::TermContext *ctx) override;
    virtual std::any visitFactor(Python3Parser::FactorContext *ctx) override;
    virtual std::any visitAtom_expr(Python3Parser::Atom_exprContext *ctx) override;
    virtual std::any visitTrailer(Python3Parser::TrailerContext *ctx) override;
    virtual std::any visitAtom(Python3Parser::AtomContext *ctx) override;
    virtual std::any visitFormat_string(Python3Parser::Format_stringContext *ctx) override;
    virtual std::any visitTestlist(Python3Parser::TestlistContext *ctx) override;
    virtual std::any visitArglist(Python3Parser::ArglistContext *ctx) override;
    virtual std::any visitArgument(Python3Parser::ArgumentContext *ctx) override;
    virtual std::any visitComp_op(Python3Parser::Comp_opContext *ctx) override;
};

#endif//PYTHON_INTERPRETER_EVALVISITOR_H
