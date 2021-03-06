// Copyright (C) 2014 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#include "STree.h"

#include "IncParser.h"
#include "Rule.h"
#include "SLog.h"

#include "util/Graphviz.h"
using Util::dotVar;

#include <memory>
#include <ostream>
#include <string>
using std::auto_ptr;
using std::ostream;
using std::string;

using namespace statik;

/* public */

STree::STree(IncParser& incParser,
               const Rule& rule,
               STree* parent)
  : m_incParser(incParser),
    m_rule(rule),
    m_parent(parent),
    m_depth(m_parent ? m_parent->GetDepth() + 1 : 0),
    m_isClear(true),
    m_parseFunc(rule.CloneParseFunc()),
    m_outputFunc(rule.CloneOutputFunc()),
    m_forceChange(false) {
  m_parseFunc->Init(*this);
  m_outputFunc->Init(*this);
}

void STree::StartNode(const List& istart) {
  g_log.info() << "Starting node " << *this << " with inode " << istart;
  if (!m_isClear) {
    throw SError("Cannot Start node that isn't clear");
  }
  m_isClear = false;
  m_iconnection.Restart(istart);
  m_incParser.DrawGraph(*this, &istart);
  m_incParser.Enqueue(ParseAction(ParseAction::Restart, *this, istart));
}

void STree::ParseNode(ParseAction::Action action, const List& inode, const STree* initiator) {
  if (m_isClear) {
    g_log.info() << "Not parsing node " << *this << " which has been cleared";
    return;
  } else if (m_iconnection.IsClear()) {
    throw SError("Cannot parse node " + Print() + " with cleared IConnection");
  } else if (ParseAction::Start == action) {
    throw SError("Cannot parse node " + Print() + " with Start action");
  }

  if (ParseAction::Restart == action) {
    m_iconnection.Restart(inode);
  }

  g_log.info() << "Parsing node " << *this << " with inode " << inode << " and initiator " << (initiator ? initiator->Print() : "<null>");
  const State old_state = m_state;
  const List* old_istart = &IStart();
  const List* old_iend = &IEnd();

  (*m_parseFunc)(action, inode, initiator);

  const List* new_istart = (m_iconnection.IsClear() ? NULL : &IStart());
  const List* new_iend = (m_iconnection.IsClear() ? NULL : &IEnd());

  bool hasChanged0 = old_istart != new_istart;
  bool hasChanged1 = old_iend != new_iend;
  bool hasChanged2 = old_state != m_state;
  bool hasChanged3 = m_forceChange;
  bool hasChanged = hasChanged0 || hasChanged1 || hasChanged2 || hasChanged3;
  g_log.info() << " - - - - " << *this << " has " << (hasChanged ? "" : "NOT ") << "changed  " << hasChanged1 << ":" << hasChanged2 << ":" << hasChanged3;
  m_incParser.TouchNode(*this);
  if (hasChanged && !m_isClear && m_parent && !m_state.IsPending()) {
    g_log.debug() << " - - - - " << " enqueueing child update due to change";
    ParseAction::Action a = ParseAction::ChildUpdate;
    m_incParser.Enqueue(ParseAction(a, *m_parent, inode, this));
  }
  m_incParser.DrawGraph(*this, &inode, NULL, initiator);
  m_incParser.ForceChange(*this);
}

void STree::ClearNode(const List& inode) {
  g_log.info() << "Clearing node " << *this;
  if (m_parent) {
    m_incParser.Enqueue(ParseAction(ParseAction::ChildUpdate, *m_parent, inode, this));
  }
  ClearSubNode();
}

string STree::Print() const {
  return m_rule.Name() + ":" + GetState().Print();
}

string STree::DrawNode(const string& context, const STree* initiator) const {
  string s;
  // Style the node to indicate its State
  string fillcolor = "#000000";
  if (m_state.IsPending()) {
    fillcolor = "#eeeeee";
  } else if (m_state.IsBad()) {
    fillcolor = "#ff8888";
  } else if (m_state.IsOK()) {
    fillcolor = "#ffeebb";
  } else if (m_state.IsDone()) {
    fillcolor = "#99ffcc";
  } else if (m_state.IsComplete()) {
    fillcolor = "#aaaaff";
  } else {
    throw SError("STree::DrawNode() - unknown state");
  }
  s += dotVar(this, context) + " [label=\"" + Util::safeLabelStr(m_rule.Name()) + "\", style=\"filled\", fillcolor=\"" + fillcolor + "\", fontsize=12.0];\n";

  // Connect the node to its IConnection
  if (!m_iconnection.IsClear()) {
    string istartcolor = "#006600";
    string iendcolor = "#660000";
    if (m_state.IsBad()) {
      istartcolor = "#66cc66";
      iendcolor = "#cc6666";
    }
    s += dotVar(this, context) + " -> " + dotVar(&m_iconnection.Start(), context) + " [constraint=false, weight=0, style=dotted, arrowsize=0.5, color=\"" + istartcolor + "\"];\n";
    s += dotVar(this, context) + " -> " + dotVar(&m_iconnection.End(), context) + " [constraint=false, weight=0, style=dotted, arrowsize=0.5, color=\"" + iendcolor + "\"];\n";
  }

  // Add its child connections, and draw the children
  // Make sure the children will be ordered in the output graph
  for (child_iter i = children.begin(); i != children.end(); ++i) {
    s += dotVar(this, context) + " -> " + dotVar(*i, context);
    if (initiator == *i) {
      s += " [penwidth=2.5, color=\"#330066\"]";
    }
    s += ";\n";
  }
  if (children.size() > 0) {
    s += "{ rank=same;\n";
    child_iter prev_child = children.begin();
    for (child_iter i = children.begin() + 1; i != children.end(); ++i) {
      s += dotVar(*prev_child, context) + " -> " + dotVar(*i, context) + " [style=\"invis\"];\n";
      prev_child = i;
    }
    s += "}\n";
  }
  for (child_iter i = children.begin(); i != children.end(); ++i) {
    s += (*i)->DrawNode(context, initiator);
  }
  return s;
}

void STree::ClearSubNode() {
  g_log.info() << "Clearing sub node " << *this;
  if (m_isClear) {
    g_log.info() << " - already clear!";
    return;
  }
  for (child_mod_iter i = children.begin(); i != children.end(); ++i) {
    (*i)->ClearSubNode();
  }
  children.clear();
  m_state.Clear();
  m_iconnection.Clear();
  m_incParser.TouchNode(*this);
  m_incParser.ClearNode(*this);
  m_isClear = true;
}

/* non-member */

ostream& statik::operator<< (ostream& out, const STree& node) {
  out << node.Print();
  return out;
}
