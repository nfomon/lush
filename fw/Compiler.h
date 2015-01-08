// Copyright (C) 2014 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#ifndef _Compiler_h_
#define _Compiler_h_

#include "Rule.h"

#include "util/Log.h"

#include <memory>

namespace fw {

/*
std::auto_ptr<Rule> CreateCompiler_Simple(Log& log);
std::auto_ptr<Rule> CreateCompiler_Moderate(Log& log);
std::auto_ptr<Rule> CreateCompiler_Complex(Log& log);
*/
std::auto_ptr<Rule> CreateCompiler_Nifty(Log& log);

}

#endif // _Compiler_h_
