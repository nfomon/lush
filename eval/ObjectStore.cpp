// Copyright (C) 2013 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#include "ObjectStore.h"

#include "EvalError.h"
#include "Object.h"

#include <boost/lexical_cast.hpp>

#include <memory>
#include <string>
using std::auto_ptr;
using std::string;

using namespace eval;

ObjectStore::~ObjectStore() {
  reset();
}

// Clears all objects from the store
void ObjectStore::reset() {
  revertAll();
  for (ordering_rev_mod_iter i = m_ordering.rbegin();
       i != m_ordering.rend(); ++i) {
    symbol_mod_iter s = m_symbols.find(i->first);
    if (m_symbols.end() == s) {
      throw EvalError("Resetting ObjectStore failed to find " + i->first);
    }
    if (s->second) {
      if (s->second->object.get()) {
        s->second->object->destruct();
      }
      delete s->second;
    }
    m_symbols.erase(s);
  }
  m_ordering.clear();
  if (!m_symbols.empty()) {
    throw EvalError("Resetting ObjectStore failed to clear all objects");
  }
}

// Commit (confirm) the oldest pending change to the store
void ObjectStore::commitFirst() {
  if (m_changeset.empty()) {
    throw EvalError("Cannot commit first of changeset; no changes pending");
  }
  SymbolChange* sc = m_changeset.front();
  symbol_iter s = m_symbols.find(sc->varname);
  if (m_symbols.end() == s) {
    throw EvalError("Cannot commit " + sc->varname + "; symbol missing");
  }
  AddSymbol* add = dynamic_cast<AddSymbol*>(sc);
  DelSymbol* del = dynamic_cast<DelSymbol*>(sc);
  if (add) {
    m_log.debug("Commit: addition of " + add->varname);
    if (!s->second->object.get()) {
      throw EvalError("Cannot commit " + add->varname + "; object missing");
    }
    s->second->object->construct();
  } else if (del) {
    m_log.debug("Commit: deletion of " + del->varname);
    if (!del->oldObject.get()) {
      throw EvalError("Cannot commit deletion of " + del->varname + "; old object missing from changeset");
    }
    bool found = false;
    for (ordering_mod_iter i = m_ordering.begin();
         i != m_ordering.end(); ++i) {
      if ((del->varname == i->first) && i->second) {
        m_ordering.erase(i);
        found = true;
        break;
      }
    }
    if (!found) {
      throw EvalError("Cannot commit deletion of " + del->varname + "; object is missing from ordering");
    }
    del->oldObject->destruct();
  } else {
    throw EvalError("Unknown SymbolChange");
  }
  delete sc;
  m_changeset.pop_front();
}

// Commit all pending-commit objects
void ObjectStore::commitAll() {
  m_log.debug("Committing all changes");
  for (size_t i=0; i < m_changeset.size(); ++i) {
    commitFirst();
  }
}

// Revert the newest pending-commit change from the store
void ObjectStore::revertLast() {
  if (m_changeset.empty()) {
    throw EvalError("Cannot revert last of changeset; no changes pending");
  }
  SymbolChange* sc = m_changeset.back();
  symbol_mod_iter s = m_symbols.find(sc->varname);
  if (m_symbols.end() == s) {
    throw EvalError("Cannot revert " + sc->varname + "; symbol missing");
  }
  AddSymbol* add = dynamic_cast<AddSymbol*>(sc);
  DelSymbol* del = dynamic_cast<DelSymbol*>(sc);
  if (add) {
    m_log.debug("Revert: addition of " + add->varname);
    // The add to revert should be the last element of m_ordering
    if (m_ordering.empty()) {
      throw EvalError("Cannot revert addition of " + add->varname + "; object is missing from ordering");
    }
    ordering_pair order = m_ordering.back();
    if (order.first != add->varname) {
      throw EvalError("Cannot revert addition of " + add->varname + "; object missing or misplaced in ordering");
    } else if (order.second) {
      throw EvalError("Cannot revert addition of " + add->varname + "; ordering suggests object is pending deletion");
    }
    m_ordering.pop_back();
    m_symbols.erase(s);
  } else if (del) {
    m_log.debug("Revert: deletion of " + del->varname);
    if (!del->oldObject.get()) {
      throw EvalError("Cannot revert deletion of " + del->varname + "; old object missing from changeset");
    }
    bool found = false;
    for (ordering_rev_mod_iter i = m_ordering.rbegin();
         i != m_ordering.rend(); ++i) {
      if (del->varname == i->first) {
        if (!i->second) {
          throw EvalError("Cannot revert deletion of " + del->varname + "; ordering suggests object is not pending deletion");
        }
        i->second = false;
        found = true;
        break;
      }
    }
    if (!found) {
      throw EvalError("Cannot revert deletion of " + del->varname + "; object is missing from ordering");
    }
    auto_ptr<Symbol> symbol(new Symbol(del->oldType));
    symbol->object = del->oldObject;
    symbol_pair sp(del->varname, symbol.release()); // TODO possible memory leak
    m_symbols.insert(make_pair(del->varname, symbol.release()));
  } else {
    throw EvalError("Unknown SymbolChange");
  }
  delete sc;
  m_changeset.pop_back();
}

// Revert all pending-commit objects
void ObjectStore::revertAll() {
  for (size_t i=0; i < m_changeset.size(); ++i) {
    revertLast();
  }
}

Object* ObjectStore::getObject(const string& varname) const {
  symbol_iter s = m_symbols.find(varname);
  if (s != m_symbols.end()) return s->second->object.get();
  return NULL;
}

void ObjectStore::newObject(const string& varname, auto_ptr<Type> type) {
  // An object name collision should have already been detected, but repeat
  // this now until we're confident about that
  if (getObject(varname)) {
    throw EvalError("Cannot create variable " + varname + "; already exists, and should never have been created");
  }
  m_log.info("Pending addition of object " + varname + " to an object store");
  symbol_pair sp(varname, new Symbol(type));
  m_symbols.insert(sp);
  // slow paranoid safety check
  for (ordering_iter i = m_ordering.begin(); i != m_ordering.end(); ++i) {
    if (varname == i->first && !i->second) {
      throw EvalError("Cannot create variable " + varname + "; somehow already exists in ordering");
    }
  }
  ordering_pair orp(varname, false);
  m_ordering.push_back(orp);
  auto_ptr<AddSymbol> as(new AddSymbol(varname));
  m_changeset.push_back(as.release());
}

void ObjectStore::delObject(const string& varname) {
  m_log.info("Pending deletion of object " + varname + " to an object store");
  symbol_mod_iter s = m_symbols.find(varname);
  if (m_symbols.end() == s) {
    throw EvalError("Cannot delete variable " + varname + "; does not exist in this store");
  }
  // this steals the type and object from s (of m_symbols)
  auto_ptr<DelSymbol> ds(new DelSymbol(varname, s->second->type, s->second->object));
  bool found = false;
  for (ordering_mod_iter i = m_ordering.begin(); i != m_ordering.end(); ++i) {
    if ((varname == i->first) && !i->second) {
      i->second = true;
      found = true;
      break;
    }
  }
  if (!found) {
    throw EvalError("Cannot delete variable " + varname + "; somehow does not exist in ordering");
  }
  delete s->second;
  m_symbols.erase(s);
  m_changeset.push_back(ds.release());
}

void ObjectStore::initObject(const string& varname,
                             auto_ptr<Object> newObject) {
  m_log.info("Initializing object " + varname);
  symbol_mod_iter s = m_symbols.find(varname);
  if (m_symbols.end() == s) {
    throw EvalError("Cannot replace variable " + varname + "; does not exist in this store");
  } else if (!s->second) {
    throw EvalError("Cannot initialize variable " + varname + " that exists but has no symbol");
  } else if (s->second->object.get()) {
    throw EvalError("Cannot initialize variable " + varname + " that already has an object");
  } else if (!newObject.get()) {
    throw EvalError("Cannot initialize variable " + varname + " with no newObject");
  }
  if (varname != newObject->getName()) {
    m_log.warning("Replacing ObjectStore name " + varname + " with object that thinks its name is " + newObject->getName());
  }
  s->second->object = newObject;
  s->second->object->construct();
}

void ObjectStore::replaceObject(const string& varname,
                                auto_ptr<Object> newObject) {
  m_log.info("Replacing object " + varname);
  symbol_mod_iter s = m_symbols.find(varname);
  if (m_symbols.end() == s) {
    throw EvalError("Cannot replace variable " + varname + "; does not exist in this store");
  } else if (!s->second || !s->second->object.get()) {
    throw EvalError("Cannot replace variable " + varname + "; found deficient symbol in the store");
  } else if (!newObject.get()) {
    throw EvalError("Cannot replace variable " + varname + " with no newObject");
  }
  if (varname != newObject->getName()) {
    m_log.warning("Replacing ObjectStore name " + varname + " with object that thinks its name is " + newObject->getName());
  }
  s->second->object->destruct();
  s->second->object = newObject;
  s->second->object->construct();
}

auto_ptr<ObjectStore> ObjectStore::duplicate() const {
  m_log.debug("Duplicating object store");
  if (!m_changeset.empty() || !m_ordering.empty()) {
    throw EvalError("Cannot duplicate ObjectStore that has pending changes");
  }
  auto_ptr<ObjectStore> os(new ObjectStore(m_log));
  for (symbol_iter i = m_symbols.begin(); i != m_symbols.end(); ++i) {
    if (!i->second || !i->second->type.get() || !i->second->object.get()) {
      throw EvalError("Cannot duplicate ObjectStore with deficient symbol");
    }
    os->newObject(i->first, i->second->type->duplicate());
    os->initObject(i->first, i->second->object->clone(i->second->object->getName()));
  }
  os->commitAll();
  for (symbol_iter i = m_symbols.begin(); i != m_symbols.end(); ++i) {
    os->replaceObject(i->first, i->second->object->clone(i->second->object->getName()));
  }
  return os;
}
