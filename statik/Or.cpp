// Copyright (C) 2014 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#include "Or.h"

#include "IncParser.h"
#include "OutputFunc.h"
#include "SLog.h"
#include "STree.h"

#include <memory>
#include <string>
#include <vector>
using std::auto_ptr;
using std::string;
using std::vector;

using namespace statik;

auto_ptr<Rule> statik::OR(const string& name) {
  return auto_ptr<Rule>(new Rule(name,
      MakeParseFunc_Or(),
      MakeOutputFunc_Winner()));
}

auto_ptr<ParseFunc> statik::MakeParseFunc_Or() {
  return auto_ptr<ParseFunc>(new ParseFunc_Or());
}

// TODO consider initiator, keep vectors as state that gets updated by each
// call to this function, and then we finally just update State from the vecs.
void ParseFunc_Or::operator() (ParseAction::Action action, const List& inode, const STree* initiator) {
  g_log.debug() << "Parsing Or at " << *m_node;

  if (ParseAction::Restart == action) {
    if (m_node->children.empty()) {
      if (m_node->GetRule().GetChildren().empty()) {
        throw SError("Or rule must have at least one child");
      }
      for (Rule::child_iter i = m_node->GetRule().GetChildren().begin();
          i != m_node->GetRule().GetChildren().end(); ++i) {
        (*i)->MakeNode(*m_node, inode);
      }
    } else if (m_node->children.size() == m_node->GetRule().GetChildren().size()) {
      for (STree::child_mod_iter i = m_node->children.begin();
           i != m_node->children.end(); ++i) {
        (*i)->GetIncParser().Enqueue(ParseAction(ParseAction::Restart, **i, inode));
      }
    } else {
      throw SError("Or node had inappropriate # children");
    }
    if (m_node->children.empty()) {
      throw SError("Failed to create children for Or node");
    }
    return;
  }

  // Compute new state flags
  State& state = m_node->GetState();
  state.Clear();
  vector<const STree*> oks;
  vector<const STree*> bads;
  vector<const STree*> dones;
  vector<const STree*> completes;
  const STree* lockedChild = NULL;

  if (m_node->children.empty()) {
    throw SError("Cannot parse Or at " + m_node->Print() + " that has no children");
  }
  m_node->GetIConnection().SetEnd(m_node->IStart());

  STree::child_mod_iter i = m_node->children.begin();
  bool haveSetEnd = false;
  for (; i != m_node->children.end(); ++i) {
    if ((*i)->IsClear() || (*i)->GetState().IsPending()) {
      // Clear ourselves!
      g_log.info() << "Or: " << *m_node << " child " << **i << " is clear or pending - giving up on entire Or!";
      m_node->ClearNode(inode);
      return;
    }
    State& istate = (*i)->GetState();
    bool thisChildLocked = false;
    if (istate.IsLocked()) {
      if (lockedChild) {
        throw SError("Parsing Or at " + m_node->Print() + " found more than one locked children");
      }
      g_log.info() << "Parsing Or at " << *m_node << " found locked child " << **i << " in state " << istate;
      lockedChild = *i;
      thisChildLocked = true;
      state.Lock();
    }
    if (!istate.IsBad() && !haveSetEnd) {
      if (&(*i)->IStart() != &m_node->IStart()) {
        throw SError("Parsing Or at " + m_node->Print() + " and a child disagree about istart");
      }
      m_node->GetIConnection().SetEnd((*i)->IEnd());
      g_log.debug() << "Parsing Or at " << *m_node << " assigning iend " << m_node->IEnd() << " from istate " << istate;
      haveSetEnd = true;
    }
    if (istate.IsOK()) {
      if (thisChildLocked) {
        bads.clear();
        oks.clear();
        dones.clear();
        completes.clear();
        oks.push_back(lockedChild);
      } else if (!lockedChild) {
        oks.push_back(*i);
      }
    } else if (istate.IsBad()) {
      if (thisChildLocked) {
        bads.clear();
        oks.clear();
        dones.clear();
        completes.clear();
        bads.push_back(lockedChild);
      } else if (!lockedChild) {
        bads.push_back(*i);
      }
    } else if (istate.IsDone()) {
      if (thisChildLocked) {
        bads.clear();
        oks.clear();
        dones.clear();
        completes.clear();
        dones.push_back(lockedChild);
      } else if (!lockedChild) {
        dones.push_back(*i);
      }
    } else if (istate.IsComplete()) {
      if (thisChildLocked) {
        bads.clear();
        oks.clear();
        dones.clear();
        completes.clear();
        completes.push_back(lockedChild);
      } else if (!lockedChild) {
        completes.push_back(*i);
      }
    } else {
      throw SError("Parsing Or at " + m_node->Print() + " found istate " + istate.Print() + " in unknown station");
    }
  }
  if (1 == completes.size()) {
    m_node->GetIConnection().SetEnd(completes.at(0)->IEnd());
    g_log.debug() << "Parsing Or at " << *m_node << " declares complete winner " << *completes.at(0);
  } else if (1 == dones.size()) {
    m_node->GetIConnection().SetEnd(dones.at(0)->IEnd());
    g_log.debug() << "Parsing Or at " << *m_node << " declares done winner " << *dones.at(0);
  }
  if (1 == completes.size()) {
    state.GoComplete();
  } else if (1 == dones.size()) {
    state.GoDone();
  } else if (dones.size() + oks.size() >= 1) {
    state.GoOK();
  } else {
    state.GoBad();
  }

  g_log.debug() << "Or now at: " << *m_node;
}
