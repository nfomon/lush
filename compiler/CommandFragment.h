// Copyright (C) 2013 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#ifndef _CommandFragment_h_
#define _CommandFragment_h_

/* CommandFragment: a piece of a command-line */

#include "Node.h"
#include "RootNode.h"
#include "Token.h"

#include "util/Log.h"

#include <string>

namespace compiler {

class CommandFragment : public Node {
public:
  CommandFragment(Log& log, RootNode*const root, const Token& token)
    : Node(log, root, token) {}
  virtual void setup();
  virtual void compile();
  virtual std::string cmdText() const;
};

}

#endif // _CommandFragment_h_
