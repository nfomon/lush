// Copyright (C) 2014 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#include "STree.h"

#include "Connector.h"
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

STree::STree(Connector& connector,
               const Rule& rule, STree* parent,
               auto_ptr<ComputeFunc> computeFunc,
               auto_ptr<OutputFunc> outputFunc)
  : m_connector(connector),
    m_rule(rule),
    m_parent(parent),
    m_isClear(true),
    depth(m_parent ? m_parent->depth + 1 : 0),
    m_computeFunc(computeFunc),
    m_outputFunc(outputFunc) {
  m_computeFunc->Init(*this);
  m_outputFunc->Init(*this);
}

void STree::StartNode(const IList& istart) {
  g_log.info() << "Starting node " << *this << " with inode " << istart;
  if (!m_isClear) {
    throw SError("Cannot Start node that isn't clear");
  }
  m_isClear = false;
  m_iconnection.Restart(istart);
  m_connector.DrawGraph(*this, &istart);
  m_connector.Enqueue(ConnectorAction(ConnectorAction::Restart, *this, istart));
}

void STree::ComputeNode(ConnectorAction::Action action, const IList& inode, const STree* initiator, int resize) {
  if (m_isClear) {
    throw SError("Cannot compute node " + string(*this) + " which has been cleared");
  } else if (m_iconnection.IsClear()) {
    throw SError("Cannot compute node " + string(*this) + " with cleared IConnection");
  } else if (ConnectorAction::Start == action) {
    throw SError("Cannot compute node " + string(*this) + " with Start action");
  }

  if (ConnectorAction::Restart == action) {
    m_iconnection.Restart(inode, resize);
  }

  g_log.info() << "Computing node " << *this << " with inode " << inode << " and initiator " << (initiator ? string(*initiator) : "<null>");
  const State old_state = m_state;
  const IList* old_iend = &IEnd();

  (*m_computeFunc)(action, inode, initiator, resize);

  const IList* new_iend = (m_iconnection.IsClear() ? NULL : &IEnd());

  bool hasChanged1 = old_iend != new_iend;
  bool hasChanged2 = old_state != m_state;
  bool hasChanged = hasChanged1 || hasChanged2;
  g_log.info() << " - - - - " << *this << " has " << (hasChanged ? "" : "NOT ") << "changed  " << hasChanged1 << ":" << hasChanged2;
  m_connector.TouchNode(*this);
  if (hasChanged && !m_isClear && m_parent && !m_state.IsPending()) {
    ConnectorAction::Action a = ConnectorAction::ChildUpdate;

    // Determine the resize amount, i.e. how much the IEnd has moved.
    // TODO this should be provided by the ComputeFunc, because this determination here is slooow.
    int resize = 0;
    if (!old_state.IsBad() && !old_state.IsPending()) {
      const IList* fwd = old_iend->right;
      const IList* bck = old_iend;
      do {
        if (fwd && fwd == new_iend) {
          break;
        } else if (bck && bck == new_iend) {
          resize = -resize;
          break;
        }
        ++resize;
        if (fwd) { fwd = fwd->right; }
        if (bck) { bck = bck->left; }
      } while (fwd || bck);
      g_log.debug() << "Computed resize of " << resize;
    } else {
      g_log.debug() << "Old state was bad, so faking resize=0";
    }

    m_connector.Enqueue(ConnectorAction(a, *m_parent, inode, resize, this));
  }
  m_connector.DrawGraph(*this, &inode, NULL, initiator);
}

void STree::ClearNode(const IList& inode) {
  g_log.info() << "Clearing node " << *this;
  if (m_parent) {
    // The size we report is irrelevant; cleared nodes do not need to report how their IEnd changes
    m_connector.Enqueue(ConnectorAction(ConnectorAction::ChildUpdate, *m_parent, inode, 0, this));
  }
  ClearSubNode();
}

/*
bool STree::ContainsINode(const IList& inode) const {
  if (m_iconnection.IsClear()) {
    return false;
  }
  for (const IList* i = &IStart(); i != NULL && i != &IEnd(); i = i->right) {
    if (&inode == i) {
      return true;
    }
  }
  return false;
}
*/

STree::operator std::string() const {
  return m_rule.Name() + ":" + string(GetState());
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
    g_log.error() << "DrawNode: unknown state";
    g_log.error() << "state is " << m_state;
    throw SError("Stree::DrawNode() - unknown state");
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
  //s += m_outputFunc->DrawEmitting(context, *this);
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
  m_connector.TouchNode(*this);
  m_connector.ClearNode(*this);
  m_isClear = true;
}

/* non-member */

ostream& statik::operator<< (ostream& out, const STree& node) {
  out << string(node);
  return out;
}
