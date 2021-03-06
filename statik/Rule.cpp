// Copyright (C) 2014 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#include "Rule.h"

#include "IncParser.h"
#include "Keyword.h"
#include "Meta.h"
#include "Or.h"
#include "Regexp.h"
#include "SError.h"
#include "STree.h"
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

using namespace statik;

/* public */

Rule::Rule(const string& name,
           auto_ptr<ParseFunc> parseFunc,
           auto_ptr<OutputFunc> outputFunc)
  : m_name(name),
    m_parseFunc(parseFunc),
    m_outputFunc(outputFunc) {
}

Rule::~Rule() {
  childOwnership_iter own_i = m_childOwnership.begin();
  for (child_iter child_i = m_children.begin(); child_i != m_children.end(); ++child_i, ++own_i) {
    if (m_childOwnership.end() == own_i) {
      throw SError("Rule " + m_name + " internal error at destruction: childOwnership not aligned with children");
    }
    if (*own_i) {
      delete *child_i;
    }
  }
}

auto_ptr<ParseFunc> Rule::CloneParseFunc() const {
  return m_parseFunc->Clone();
}

auto_ptr<OutputFunc> Rule::CloneOutputFunc() const {
  return m_outputFunc->Clone();
}

Rule& Rule::SetParseFunc(auto_ptr<ParseFunc> parseFunc) {
  m_parseFunc = parseFunc;
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

STree* Rule::MakeNode(STree& parent, const List& istart) const {
  return MakeNode(parent, istart, parent.children.end());
}

STree* Rule::MakeNode(STree& parent, const List& istart, STree::child_mod_iter insertPos) const {
  auto_ptr<STree> node(new STree(parent.GetIncParser(), *this, &parent));
  g_log.debug() << "Rule " << m_name << " making node " << *node.get() << " under parent that had " << parent.children.size() << " children";
  STree* r = parent.GetIncParser().OwnNode(node);
  parent.children.insert(insertPos, r);
  parent.GetIncParser().Enqueue(ParseAction(ParseAction::Start, *r, istart));
  return r;
}

Rule* Rule::AddChild(auto_ptr<Rule> child) {
  m_children.push_back(child.release());
  m_childOwnership.push_back(true);
  return m_children.back();
}

Rule* Rule::AddChildRecursive(Rule* child) {
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
        throw SError("Rule " + m_name + " internal error in Rule::Print(): childOwnership not aligned with children");
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
      throw SError("Rule " + m_name + " internal error in Rule::DrawNode(): childOwnership not aligned with children");
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

ostream& statik::operator<< (ostream& out, const Rule& rule) {
  out << rule.Name();
  return out;
}
