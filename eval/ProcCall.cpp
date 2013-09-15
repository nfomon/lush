// Copyright (C) 2013 Michael Biggs.  See the LICENSE file at the top-level
// directory of this distribution and at http://lush-shell.org/copyright.html

#include "ProcCall.h"

#include "Expression.h"
#include "EvalError.h"
#include "Function.h"

#include <memory>
#include <string>
#include <vector>
using std::auto_ptr;
using std::string;
using std::vector;

using namespace eval;

void ProcCall::setup() {
  if (children.size() < 1) {
    throw EvalError("ProcCall must have >= 1 children");
  }
  Variable* var = dynamic_cast<Variable*>(children.at(0));
  if (!var) {
    throw EvalError("ProcCall first child must be a Variable");
  }
  Object& object = var->getObject();
  m_function = dynamic_cast<Function*>(&object);
  if (!m_function) {
    throw EvalError("ProcCall cannot call a non-function");
  }
  type_list argtypes;
  for (child_iter i = children.begin()+1; i != children.end(); ++i) {
    Expression* exp = dynamic_cast<Expression*>(*i);
    if (!exp) {
      throw EvalError("ProcCall args must be Expressions");
    }
    m_argexps.push_back(exp);
    argtypes.push_back(&exp->type());
  }
  // Verify that the function has a signature for these arg types
  if (!m_function->takesArgs(argtypes)) {
    throw EvalError("Function " + m_function->print() + " does not accept the provided arguments");
  }
}

void ProcCall::evaluate() {
  // The expressions of m_argexps have all been evaluated.  Call the
  // function on their resulting objects.
  object_list args;
  for (vector<Expression*>::const_iterator i = m_argexps.begin();
       i != m_argexps.end(); ++i) {
    args.push_back(&(*i)->getObject());
  }
  auto_ptr<Object> ret = m_function->call(args);
}

void ProcCall::computeType() {
  // ProcCall type is the return type of the function
  //m_type.reset(m_signature->getType().duplicate());
}
