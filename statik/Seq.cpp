// Copyright (C) 2014 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#include "Seq.h"

#include "Connector.h"
#include "OutputFunc.h"
#include "RestartFunc.h"
#include "SLog.h"
#include "STree.h"

#include <boost/lexical_cast.hpp>
using boost::lexical_cast;

#include <memory>
#include <string>
#include <vector>
using std::auto_ptr;
using std::string;
using std::vector;

using namespace statik;

auto_ptr<Rule> statik::SEQ(const string& name) {
  return auto_ptr<Rule>(new Rule(name,
      MakeRestartFunc_FirstChildOfNode(),
      MakeComputeFunc_Seq(),
      MakeOutputFunc_Sequence()));
}

auto_ptr<ComputeFunc> statik::MakeComputeFunc_Seq() {
  return auto_ptr<ComputeFunc>(new ComputeFunc_Seq());
}

void ComputeFunc_Seq::operator() () {
  g_log.debug() << "Computing Seq at " << *m_node;

  // Initialize state flags
  State& state = m_node->GetState();
  state.Clear();

  if (m_node->children.empty()) {
    throw SError("SeqRule::Update: Seq node " + string(*m_node) + " must have children");
  } else if (m_node->children.size() > m_node->GetRule().GetChildren().size()) {
    throw SError("SeqRule::Update: Seq node " + string(*m_node) + " has more children than the rule");
  }
  m_node->GetIConnection().Restart(m_node->children.at(0).IStart());

  // Iterate over node children, either existing or being created, so long as
  // our last child is complete.
  bool finished = false;
  unsigned int child_index = 0;
  STree::child_mod_iter child = m_node->children.begin();
  STree* prev_child = NULL;
  while (!finished) {
    if (child_index >= m_node->GetRule().GetChildren().size()) {
      throw SError("Computing Seq at " + string(*m_node) + " evaluated too many children beyond the rule");
    }
    if (child != m_node->children.end()) {
      // Existing child
      if (prev_child) {
        if (!child->IStart().left) {
          throw SError("Computing Seq  at " + string(*m_node) + " child " + string(*child) + " has istart at the start of input, but it's not our first child");
        }
        if (&prev_child->IEnd() != &child->IStart()) {
          g_log.info() << "Computing Seq at " << *m_node << " child " << *child << " needs to be repositioned to the node after the prev child's end";
          child->RestartNode(prev_child->IEnd());
        }
      }
    } else if (m_node->children.size() > m_node->GetRule().GetChildren().size()) {
      throw SError("Computing Seq node " + string(*m_node) + " has more children than the rule");
    } else if (child_index >= m_node->GetRule().GetChildren().size()) {
      throw SError("Computing Seq node at " + string(*m_node) + ": child index >= Rule size");
    } else {
      // New child
      const IList* newIStart = &m_node->IStart();
      if (prev_child) {
        newIStart = &prev_child->IEnd();
      }
      if (!newIStart) {
        throw SError("Computing Seq at " + string(*m_node) + (prev_child ? (" with prev child " + string(*prev_child)) : " with no prev child") + " failed to find new istart for new child");
      }
      (void) m_node->GetRule().GetChildren().at(child_index)->MakeNode(*m_node, *newIStart);
      child = m_node->children.end() - 1;
    }
    m_node->GetIConnection().SetEnd(child->IEnd());

    // Now check the child's state, and see if we can keep going.
    State& istate = child->GetState();

    if (istate.IsLocked()) {
      state.Lock();
    }

    if (istate.IsBad()) {
      g_log.debug() << "Computing Seq at " << *m_node << " has gone bad";
      state.GoBad();
      // Clear any subsequent children
      for (STree::child_mod_iter i = child+1; i != m_node->children.end(); ++i) {
        i->ClearNode();
      }
      m_node->children.erase(child+1, m_node->children.end());
      finished = true;
    } else if (istate.IsComplete()) {
      // Are we complete at the end of our sequence?
      if (child_index == m_node->GetRule().GetChildren().size() - 1) {
        state.GoComplete();
        finished = true;
      } else {
        // Cool, keep going!
      }
    } else if (istate.IsAccepting()) {
      if (child->IEnd().right) {
        throw SError("Computing Seq at " + string(*m_node) + " found incomplete inode that is only ok; not allowed");
      }
      if (istate.IsDone() && child_index == m_node->GetRule().GetChildren().size() - 1) {
        state.GoDone();
      } else {
        state.GoOK();
      }
      // Clear any subsequent children
      for (STree::child_mod_iter i = child+1; i != m_node->children.end(); ++i) {
        i->ClearNode();
      }
      m_node->children.erase(child+1, m_node->children.end());
      finished = true;
    } else {
      throw SError("Computing Seq at " + string(*m_node) + " child is in unexpected state");
    }

    prev_child = &*child;
    ++child;
    ++child_index;
  }

  if (!prev_child) {
    throw SError("Computing Seq at " + string(*m_node) + " should have assigned a previous child at some point");
  }

  g_log.debug() << "Computing Seq at " << *m_node << " done update; now has state " << *m_node;
  g_log.debug() << " - and it has istart " << m_node->IStart() << " and iend " << m_node->IEnd();
}