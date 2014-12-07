// Copyright (C) 2014 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#ifndef _Or_h_
#define _Or_h_

#include "Rule.h"
#include "State.h"

#include <memory>

namespace fw {

struct OrRule : public Rule {
  OrRule(Log& log, const std::string& name = "")
    : Rule(log, name) {}
  virtual ~OrRule() {}
  virtual void Reposition(Connector& connector, TreeDS& x, const IList& inode) const;
  virtual bool Update(Connector& connector, TreeDS& x, const TreeDS* child) const;
  virtual std::auto_ptr<State> MakeState() const;
};

struct OrState : public State {
  OrState(const OrRule& rule)
    : State(rule) {}
  virtual ~OrState() {}

  virtual operator std::string() const { return "Or " + rule.Name() + ":" + Print(); }
};

}

#endif // _Or_h_
