// Copyright (C) 2014 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#include "Connector.h"

#include "FWTree.h"
#include "Grapher.h"
#include "Hotlist.h"
#include "IList.h"
#include "Rule.h"

#include <boost/lexical_cast.hpp>
using boost::lexical_cast;

#include <set>
#include <string>
using std::set;
using std::string;

using namespace fw;

/* public */

Connector::Connector(Log& log, const Rule& rule, const std::string& name, Grapher* grapher)
  : m_log(log),
    m_rule(rule),
    m_name(name),
    m_grapher(grapher) {
}

const Hotlist::hotlist_vec& Connector::GetHotlist() const {
  return GetRoot().GetOutputFunc().GetHotlist();
}
string Connector::PrintHotlist() const {
  return GetRoot().GetOutputFunc().PrintHotlist();
}

void Connector::Insert(const IList& inode) {
  m_log.info("Connector " + m_name + ": Inserting IList: " + string(inode));
  ResetNodes();

  if (!m_root.get()) {
    m_log.debug(" - No root; initializing the tree root");
    m_root = m_rule.MakeRootNode(*this);
    m_root->RestartNode(inode);
  } else {
    if (inode.left) {
      m_log.debug(" - Found left inode: updating listeners of " + string(inode) + "'s left and (if present) right");
      UpdateListeners(inode, true);
    } else {
      // Just reposition the root.
      m_log.debug(" - No left inode: just repositioning the root");
      m_root->RestartNode(inode);
    }
  }

  // Assert that the inode has at least one listener in the updated set.
  if (!m_listeners.HasAnyListeners(&inode)) {
    m_root->GetState().GoBad();
  }
  // Stronger check: every inode has at least one listener :)

  m_log.debug("Connector: Insert done, hotlist now has size " + boost::lexical_cast<string>(GetHotlist().size()));
}

void Connector::Delete(const IList& inode) {
  m_log.info("Deleting list inode " + string(inode));
  ResetNodes();

  if (!m_root.get()) {
    throw FWError("Connector " + m_name + ": cannot delete " + string(inode) + " when the root has not been initialized");
  }

  m_listeners.RemoveAllListeners(&inode);

  if (!inode.left && !inode.right) {
    m_root->ClearNode();
  } else if (!inode.left) {
    m_root->RestartNode(*inode.right);
  } else {
    UpdateListeners(inode, true);
  }

  m_log.debug("Connector: Delete done, hotlist now has size " + boost::lexical_cast<string>(GetHotlist().size()));
}

void Connector::UpdateWithHotlist(const Hotlist::hotlist_vec& hotlist) {
  for (Hotlist::hotlist_iter i = hotlist.begin(); i != hotlist.end(); ++i) {
    switch (i->second) {
    case Hotlist::OP_INSERT:
      Insert(*i->first);
      break;
    case Hotlist::OP_DELETE:
      Delete(*i->first);
      break;
    case Hotlist::OP_UPDATE:
      UpdateListeners(*i->first, false);
      break;
    default:
      throw FWError("Cannot update with hotlist with unknown hot operation");
    }
  }
}

void Connector::ClearNode(FWTree& x) {
  m_listeners.RemoveAllListenings(&x);
}

void Connector::Listen(FWTree& x, const IList& inode) {
  if (m_listeners.IsListening(&inode, &x)) {
    m_log.info("Connector: " + std::string(x) + " is already listening to " + std::string(inode));
  } else {
    m_log.info("Connector: " + std::string(x) + " will listen to " + std::string(inode));
    m_listeners.AddListener(&inode, &x);
  }
}

void Connector::Unlisten(FWTree& x, const IList& inode) {
  m_log.info("Connector: " + std::string(x) + " will NOT listen to " + std::string(inode));
  m_listeners.RemoveListener(&inode, &x);
}

/* private */

void Connector::UpdateListeners(const IList& inode, bool updateNeighbourListeners) {
  typedef std::set<FWTree*> change_set;
  typedef change_set::const_iterator change_iter;
  typedef std::map<FWTree::depth_t, change_set> change_map;
  change_map changes_by_depth;

  if (updateNeighbourListeners) {
    if (!inode.left) {
      throw FWError("Cannot update listeners of inode " + string(inode) + " with nothing to its left");
    }

    // TODO Unnecessary copy
    listener_set left_listeners = m_listeners.GetListeners(inode.left);
    for (listener_iter i = left_listeners.begin();
         i != left_listeners.end(); ++i) {
      changes_by_depth[(*i)->depth].insert(*i);
    }

    // If inode.right and inode.right != inode.left, merge the left and right
    // listeners, depth-ordered.
    if (inode.right && inode.right != inode.left) {
      // TODO Unnecessary copy
      listener_set right_listeners = m_listeners.GetListeners(inode.right);
      for (listener_iter i = right_listeners.begin();
           i != right_listeners.end(); ++i) {
        changes_by_depth[(*i)->depth].insert(*i);
      }
    }
  } else {
    // TODO Unnecessary copy
    listener_set listeners = m_listeners.GetListeners(&inode);
    for (listener_iter i = listeners.begin(); i != listeners.end(); ++i) {
      changes_by_depth[(*i)->depth].insert(*i);
    }
  }

  if (changes_by_depth.empty()) {
    m_log.info("Connector: No " + string(updateNeighbourListeners ? "neighbouring" : "direct") + " listeners of " + string(inode) + " (left: " + (inode.left ? string(*inode.left) : "<null>") + "; right: " + (inode.right ? string(*inode.right) : "<null>") + ") to update.");
  }

  while (!changes_by_depth.empty()) {
    // Update the deepest depth of our changeset
    int depth = changes_by_depth.rbegin()->first;
    change_set changes = changes_by_depth.rbegin()->second;
    m_log.debug("Connector: Updating changes at depth " + lexical_cast<string>(depth));
    for (change_iter i = changes.begin(); i != changes.end(); ++i) {
      bool changed = (*i)->ComputeNode();
      if (changed) {
        FWTree* parent = (*i)->GetParent();
        if (parent) {
          changes_by_depth[parent->depth].insert(parent);
        }
      }
    }
    changes_by_depth.erase(depth);
  }
}

void Connector::DrawGraph(const FWTree& onode, const IList* inode) {
  if (!m_grapher) {
    return;
  }
  if (!m_root.get()) {
    return;
  }
  m_grapher->AddIList(m_name, m_root->IStart(), m_name + " input");
  m_grapher->AddOTree(m_name, *m_root.get(), m_name + " output");
  m_grapher->AddIListeners(m_name, *this, m_root->IStart());
  if (inode) {
    m_grapher->Signal(m_name, &onode);
    m_grapher->Signal(m_name, inode);
  } else {
    m_grapher->Signal(m_name, &onode, true);
  }
  m_grapher->SaveAndClear();
}

void Connector::AddNodeToReset(FWTree& x) {
  m_nodesToReset.insert(&x);
}

void Connector::ResetNodes() {
  for (set<FWTree*>::const_iterator i = m_nodesToReset.begin();
       i != m_nodesToReset.end(); ++i) {
    (*i)->GetOutputFunc().Reset();
  }
  m_nodesToReset.clear();
}
