// Copyright (C) 2014 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#include "Member.h"

#include "VMError.h"

#include "util/Util.h"

#include <string>
#include <utility>
#include <vector>
using std::pair;
using std::string;
using std::vector;

using namespace vm;

const std::string Member::NO_SOURCE = "-";