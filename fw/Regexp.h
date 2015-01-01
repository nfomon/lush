// Copyright (C) 2014 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#ifndef _Regexp_h_
#define _Regexp_h_

/* Regexp rule */

#include "ComputeFunc.h"
#include "Rule.h"

#include "util/Log.h"

#include <boost/regex.hpp>

#include <memory>
#include <string>

namespace fw {

std::auto_ptr<Rule> MakeRule_Regexp(Log& log,
    const std::string& name,
    const boost::regex& regex);

class ComputeFunc_Regexp : public ComputeFunc {
public:
  ComputeFunc_Regexp(Log& log, const boost::regex& regex);
  virtual ~ComputeFunc_Regexp() {}
  virtual void operator() ();
  virtual std::auto_ptr<ComputeFunc> Clone() {
    return std::auto_ptr<ComputeFunc>(new ComputeFunc_Regexp(m_log, m_regex));
  }

private:
  const boost::regex m_regex;
};

}

#endif // _Regexp_h_
