// Copyright (C) 2014 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#include "Variable.h"

#include "util/Util.h"

#include <boost/lexical_cast.hpp>

#include <string>
#include <utility>
using std::string;

using namespace compiler;

Variable::Variable()
  : m_scope(NULL) {
}

void Variable::init(Scope& scope) {
  m_scope = &scope;
}

void Variable::attach_name(const std::string& name) {
  m_name = name;
  m_type = m_scope->find(name);
  if (!m_type) {
    throw CompileError("Variable " + name + " does not exist in scope at depth " + boost::lexical_cast<string>(m_scope->depth()));
  }
}

void Variable::attach_member(const std::string& member) {
  if (!m_type) {
    throw CompileError("Cannot attach member " + member + " to variable " + m_name + " that is missing a type");
  }
  m_type = m_type->findMember(member);
  if (!m_type) {
    throw CompileError("Variable member " + fullname() + " does not exist in scope at depth " + boost::lexical_cast<string>(m_scope->depth()));
  }
  m_members.push_back(member);
}

std::string Variable::fullname() const {
  std::string name = m_name;
  for (member_iter i = m_members.begin(); i != m_members.end(); ++i) {
    name += "." + *i;
  }
  return name;
}

const Type& Variable::type() const {
  if (!m_type) {
    throw CompileError("Cannot get type of untyped Variable " + fullname());
  }
  return *m_type;
}