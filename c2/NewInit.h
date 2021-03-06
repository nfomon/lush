// Copyright (C) 2014 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#ifndef _NewInit_h_
#define _NewInit_h_

/* NewInit */

#include "Common.h"
#include "Expression.h"
#include "Scope.h"
#include "Type.h"

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/spirit/include/phoenix_bind.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_object.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/qi.hpp>
namespace phoenix = boost::phoenix;
namespace spirit = boost::spirit;
namespace ascii = spirit::ascii;
namespace qi = spirit::qi;

#include <memory>
#include <string>
#include <vector>

namespace compiler {

class Expression;
class Type;

class NewInit {
public:
  NewInit();
  void init(Scope& scope);
  void attach_name(const std::string& name);
  void attach_type(const Expression& exp);
  void attach_exp(const Expression& exp);
  void finalize();
  std::string name() const { return m_name; }
  const Type& type() const { return *m_type; }
  std::string bytecode_asNew() const;
  std::string bytecode_asMember() const;

private:
  Scope* m_scope;
  std::string m_name;
  boost::shared_ptr<Type> m_type;
  boost::shared_ptr<Expression> m_exp;
  std::string m_bytecode;
};

template <typename Iterator>
struct NewInitParser : qi::grammar<Iterator, NewInit(Scope&), ascii::space_type> {
public:
  NewInitParser(ExpParser<Iterator>& exp_)
    : NewInitParser::base_type(newinit_, "NewInit"),
      exp_(exp_) {
    using phoenix::ref;
    using qi::_1;
    using qi::_r1;
    using qi::_val;
    using qi::char_;
    using qi::lit;

    identifier_ %= lit("ID:'") > +char_("0-9A-Za-z_") > lit('\'');
    newinit_ = (
      lit("(init")[phoenix::bind(&NewInit::init, _val, _r1)]
      > identifier_[phoenix::bind(&NewInit::attach_name, _val, _1)]
      > -exp_(_r1, std::string("type"))[phoenix::bind(&NewInit::attach_type, _val, _1)]
      > -exp_(_r1, std::string("exp"))[phoenix::bind(&NewInit::attach_exp, _val, _1)]
      > lit(")")[phoenix::bind(&NewInit::finalize, _val)]
    );

    //BOOST_SPIRIT_DEBUG_NODE(newinit_);
  }

private:
  ExpParser<Iterator>& exp_;

  qi::rule<Iterator, std::string(), ascii::space_type> identifier_;
  qi::rule<Iterator, NewInit(Scope&), ascii::space_type> newinit_;
};

}

#endif // _NewInit_h_
