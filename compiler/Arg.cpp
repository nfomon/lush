// Copyright (C) 2014 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#include "Arg.h"

#include "CompileError.h"
#include "Identifier.h"
#include "TypeSpec.h"

#include <memory>
#include <string>
#include <vector>
using std::auto_ptr;
using std::string;
using std::vector;

using namespace compiler;

void Arg::setup() {
  // If two children, the first is the name.  Otherwise, name is "".
  if (1 == children.size()) {
    m_typeSpec = dynamic_cast<TypeSpec*>(children.at(0));
  } else if (2 == children.size()) {
    m_typeSpec = dynamic_cast<TypeSpec*>(children.at(1));
    Identifier* ident = dynamic_cast<Identifier*>(children.at(0));
    if (!ident) {
      throw CompileError("First child of 2-child Arg " + print() + " must be an Identifier");
    }
    m_argname = ident->getName();
  } else {
    throw CompileError("Arg node must have 1 or 2 children");
  }
  if (!m_typeSpec) {
    throw CompileError("Arg " + print() + " must have a TypeSpec child");
  }
}

string Arg::getName() const {
  if (!isSetup) {
    throw CompileError("Cannot get name of Arg " + print() + " before it is setup");
  }
  return m_argname;
}

auto_ptr<Type> Arg::getType() const {
  return m_typeSpec->getType();
}

const Type& Arg::type() const {
  return m_typeSpec->type();
}

auto_ptr<ArgSpec> Arg::getSpec() const {
  return auto_ptr<ArgSpec>(new ArgSpec(getName(), *getType().release()));
}
