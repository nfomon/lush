// Copyright (C) 2013 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#include "util/Proc.h"

#include <boost/lexical_cast.hpp>

#include <iostream>
#include <string>
using namespace std;

namespace {
  const string PROGRAM_NAME = "test_lexer";
  unsigned line_number = 0;
  unsigned num_tests = 0;
};

bool test(Proc& p, const string& in, const string& expected) {
  ++num_tests;
  ++line_number;
  string exp(boost::lexical_cast<string>(line_number));
  exp += " " + expected;
  p.out << in << endl;
  string obs;
  std::getline(p.in, obs);
  if (obs == exp) {
    cout << "pass: " << in << endl;
    return true;
  }
  cout << "FAIL: " << in << endl;
  cout << " - expected: '" << exp << "'" << endl;
  cout << " - observed: '" << obs << "'" << endl;
  return false;
}

bool expectFail(Proc& p, const string& in, const string& expected) {
  p.out << in << endl;
  return false;
}

int main(int argc, char* argv[]) {
  if (argc != 1) {
    cout << "usage: " << PROGRAM_NAME << endl;
    return 1;
  }

  Proc lexer("./shok_lexer");
  lexer.run();

  // Braces
  test(lexer, "{{{}{{}}}}", "1:LBRACE 2:LBRACE 3:LBRACE 4:RBRACE 5:LBRACE 6:LBRACE 7:RBRACE 8:RBRACE 9:RBRACE 10:RBRACE");

  cout << endl;
  cout << "----------" << endl;
  cout << "Ran " << num_tests << " test" << (1==num_tests?"":"s") << endl;
  cout << endl;

  lexer.finish();

  if (-1 == wait(NULL)) {
    perror("waiting for child lexer");
    _exit(-1);
  }
  return 0;
}
