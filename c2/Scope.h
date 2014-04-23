// Copyright (C) 2014 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#ifndef _Scope_h_
#define _Scope_h_

/* Scope
 *
 * Scopes form a tree of symbol tables.  Each has a local symbol table, and
 * knows how to defer non-local lookups up the tree.
 *
 * Function and Object scopes can also look up members in their enclosing
 * object or function.
 */

#include "CompileError.h"
#include "SymbolTable.h"
#include "Type.h"

#include "util/Log.h"

#include <string>
#include <vector>

namespace compiler {

class Scope {
public:
  typedef int Depth;

  Scope(Log& log, Scope* parent = NULL)
    : m_log(log),
      m_parent(parent),
      m_depth(parent ? (parent->depth() + 1) : 0),
      m_symbolTable(log) {
  }
  virtual ~Scope() {}

  Depth depth() const {
    return m_depth;
  }

  void insert(const std::string& name, const Type& type);
  const Type* find(const std::string& name) const;
  const Type* findLocal(const std::string& name) const;

private:
  Log& m_log;
  Scope* m_parent;
  Depth m_depth;
  SymbolTable m_symbolTable;
};

class ObjectScope : public Scope {
public:
private:
  //Object* m_object;
};

class FunctionScope : public Scope {
public:
private:
  //Function* m_function;
};

}

#endif // _Scope_h_
