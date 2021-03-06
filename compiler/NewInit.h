// Copyright (C) 2013 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#ifndef _NewInit_h_
#define _NewInit_h_

/* New statement's variable-initialization helper
 *
 * There are four forms of NewInit statement-fragments:
 *    new x           equivalent to   new x : object = object
 *    new x : y       equivalent to   new x : y = y
 *    new x = y       equivalent to   new x : typeof(y) = y
 *    new x : y = z   equivalent to   new x : y = z
 */

#include "Expression.h"
#include "Identifier.h"
#include "Node.h"
#include "RootNode.h"
#include "Scope.h"
#include "Token.h"
#include "TypeSpec.h"

#include "util/Log.h"

#include <string>

namespace compiler {

class NewInit : public Node {
public:
  NewInit(Log& log, RootNode*const root, const Token& token)
    : Node(log, root, token),
      m_isPrepared(false),
      m_identifier(NULL),
      m_exp(NULL),
      m_typeSpec(NULL),
      m_type(NULL) {}
  ~NewInit();

  virtual void setup();
  // Prepare the scope to include the new variable.  This is called by an
  // analysis step kickstarted by our parent New, after its setup() is
  // completed, or by a parent ObjectLiteral near the end of its setup().
  // We could do this same work at setup() time but it seems nicer to let the
  // New statement finish its setup() validation beforehand.  Here we actually
  // perform the object creation, but mark it as "pending" in the Scope until
  // compile() time finally marks it as commit().
  void prepare();
  // Commit the object to the Scope, and assign its initial value
  virtual void compile();

  std::string getName() const { return m_varname; }   // called by ObjectLiteral
  std::auto_ptr<Type> getType() const;                // called by ObjectLiteral
  std::auto_ptr<Object> getObject() const;            // called by ObjectLiteral

private:
  bool m_isPrepared;
  std::string m_varname;
  // child 0: the identifier of the variable being created
  Identifier* m_identifier;
  // child 1 or 2: the expression that compiles to the initial value.
  // In the absense of an explicit type specifier, this also gives us our type.
  Expression* m_exp;
  // child 1 if there are 2 children: explicit type specifier
  TypeSpec* m_typeSpec;
  // Type of the new variable.  Provided by m_typeSpec if it exists, otherwise
  // by the type of m_exp if it exists, otherwise stdlib::object.
  std::auto_ptr<Type> m_type;
};

}

#endif // _NewInit_h_
