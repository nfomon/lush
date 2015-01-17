// Copyright (C) 2014 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#include "Rule.h"

#include "Connector.h"
#include "FWError.h"
#include "FWTree.h"
#include "Keyword.h"
#include "Meta.h"
#include "Or.h"
#include "Regexp.h"
#include "Seq.h"
#include "Star.h"

#include "util/Graphviz.h"
using Util::dotVar;
using Util::safeLabelStr;

#include <memory>
#include <ostream>
#include <string>
using std::auto_ptr;
using std::ostream;
using std::string;

using namespace fw;

/* public */

Rule::Rule(const string& debugName, auto_ptr<RestartFunc> restartFunc, auto_ptr<ComputeFunc> computeFunc, auto_ptr<OutputFunc> outputFunc)
  : m_name(debugName),
    m_restartFunc(restartFunc),
    m_computeFunc(computeFunc),
    m_outputFunc(outputFunc),
    m_parent(NULL) {
}

Rule::~Rule() {
  childOwnership_iter own_i = m_childOwnership.begin();
  for (child_iter child_i = m_children.begin(); child_i != m_children.end(); ++child_i, ++own_i) {
    if (m_childOwnership.end() == own_i) {
      throw FWError("Rule " + string(*this) + " internal error at destruction: childOwnership not aligned with children");
    }
    if (*own_i) {
      delete *child_i;
    }
  }
}

Rule& Rule::SetRestartFunc(auto_ptr<RestartFunc> restartFunc) {
  m_restartFunc = restartFunc;
  return *this;
}

Rule& Rule::SetComputeFunc(auto_ptr<ComputeFunc> computeFunc) {
  m_computeFunc = computeFunc;
  return *this;
}

Rule& Rule::SetOutputFunc(auto_ptr<OutputFunc> outputFunc) {
  m_outputFunc = outputFunc;
  return *this;
}

Rule* Rule::SilenceOutput() {
  m_outputFunc = MakeOutputFunc_Silent();
  return this;
}

Rule* Rule::CapOutput(const string& cap) {
  m_outputFunc = MakeOutputFunc_Cap(m_outputFunc, cap);
  return this;
}

auto_ptr<FWTree> Rule::MakeRootNode(Connector& connector) const {
  auto_ptr<FWTree> node(new FWTree(connector, *this, NULL, m_restartFunc->Clone(), m_computeFunc->Clone(), m_outputFunc->Clone()));
  return node;
}

FWTree* Rule::MakeNode(FWTree& parent, const IList& istart) const {
  auto_ptr<FWTree> node(new FWTree(parent.GetConnector(), *this, &parent, m_restartFunc->Clone(), m_computeFunc->Clone(), m_outputFunc->Clone()));
  FWTree* r = node.get();
  parent.children.push_back(node);
  r->RestartNode(istart);
  return r;
}

Rule* Rule::AddChild(auto_ptr<Rule> child) {
  child->setParent(this);
  m_children.push_back(child.release());
  m_childOwnership.push_back(true);
  return m_children.back();
}

Rule* Rule::AddChildRecursive(Rule* child) {
  child->setParent(this);
  m_children.push_back(child);
  m_childOwnership.push_back(false);
  return m_children.back();
}

string Rule::Print() const {
  string s(m_name);
  if (!m_children.empty()) {
    s += " (";
    childOwnership_iter own_i = m_childOwnership.begin();
    for (child_iter i = m_children.begin(); i != m_children.end(); ++i, ++own_i) {
      if (m_childOwnership.end() == own_i) {
        throw FWError("Rule " + string(*this) + " internal error in Rule::Print(): childOwnership not aligned with children");
      }
      if (i != m_children.begin()) { s += ", "; }
      if (*own_i) {
        s += (*i)->Print();
      } else {
        s += (*i)->Name();
      }
    }
    s += ")";
  }
  return s;
}

string Rule::DrawNode(const string& context) const {
  string s;
  s += dotVar(this, context) + " [label=\"" + safeLabelStr(m_name) + "\"];\n";
  childOwnership_iter own_i = m_childOwnership.begin();
  for (child_iter i = m_children.begin(); i != m_children.end(); ++i, ++own_i) {
    if (m_childOwnership.end() == own_i) {
      throw FWError("Rule " + string(*this) + " internal error in Rule::DrawNode(): childOwnership not aligned with children");
    }
    if (*own_i) {
      s += dotVar(this, context) + " -> " + dotVar(*i, context) + ";\n";
      s += (*i)->DrawNode(context);
    } else {
      s += dotVar(this, context) + " -> " + dotVar(*i, context) + "_rec;\n";
      s += dotVar(*i, context) + "_rec [label=\"\\[ " + safeLabelStr((*i)->Name()) + " \\]\"];\n";
    }
  }
  return s;
}

/* non-member */

ostream& fw::operator<< (ostream& out, const Rule& rule) {
  out << string(rule);
  return out;
}
