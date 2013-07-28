// Copyright (C) 2013 Michael Biggs.  See the LICENSE file at the top-level
// directory of this distribution and at http://lush-shell.org/copyright.html

#ifndef _NewInit_h_
#define _NewInit_h_

/* New statement's variable-initialization helper */

#include "Log.h"
#include "Node.h"
#include "Object.h"
#include "RootNode.h"
#include "Scope.h"
#include "Token.h"
#include "Type.h"
#include "Variable.h"

#include <string>

namespace eval {

class NewInit : public Node {
public:
  NewInit(Log& log, RootNode*const root, const Token& token)
    : Node(log, root, token),
      m_prepared(false),
      m_value(NULL) {}
  ~NewInit();

  virtual void setup();
  void prepare();
  virtual void evaluate();

private:
  bool m_prepared;
  std::string m_varname;
  //Type m_type;
  Variable* m_value;
};

};

#endif // _NewInit_h_
