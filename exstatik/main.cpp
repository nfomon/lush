// Copyright (C) 2014 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

/* statik compiler framework runner */

#include "Codegen.h"
#include "Compiler.h"
#include "Lexer.h"
#include "Parser.h"

#include "statik/Batch.h"
#include "statik/Grapher.h"
#include "statik/IncParser.h"
#include "statik/List.h"
#include "statik/Rule.h"
#include "statik/SError.h"
#include "statik/SLog.h"

#include <boost/program_options.hpp>
namespace po = boost::program_options;

#include <iostream>
#include <memory>
#include <string>
using std::auto_ptr;
using std::cin;
using std::cout;
using std::endl;
using std::string;

using namespace exstatik;
using namespace statik;

namespace {
  const string PROGRAM_NAME = "exstatik";
}

int main(int argc, char *argv[]) {
  try {
    // Retrieve program options
    string compilerName;
    string logfile;
    string loglevel = Log::UnMapLevel(Log::INFO);
    string graphdir;
    po::options_description desc(PROGRAM_NAME + " usage");
    desc.add_options()
      ("help,h", "show help message")
      ("compiler,c", po::value<string>(&compilerName), "compiler name")
      ("logfile,f", po::value<string>(&logfile), "output log file")
      ("loglevel,L", po::value<string>(&loglevel), "log level: debug, info, warning, error")
      ("graphdir,g", po::value<string>(&graphdir), "output graph directory")
    ;
    po::positional_options_description p;
    p.add("compiler", 1);
    po::variables_map vm;

    try {
      po::store(po::command_line_parser(argc, argv).options(desc).positional(p).run(), vm);
      po::notify(vm);

      if (vm.count("help")) {
        cout << desc;
        return 0;
      }
    } catch (po::error& e) {
      cout << desc;
      return 1;
    }

    // Initialize logging
    if (!logfile.empty()) {
      g_log.setLevel(loglevel);
      g_log.Init(logfile);
    }

    auto_ptr<Compiler> compiler = MakeCompiler(compilerName);
    if (!compiler.get() || compiler->empty()) {
      throw statik::SError("Empty compiler; nothing to do");
    }
    typedef boost::ptr_vector<IncParser> parser_vec;
    typedef parser_vec::iterator parser_mod_iter;
    parser_vec parsers;
    for (size_t i = 0; i < compiler->size(); ++i) {
      Compiler::auto_type c = compiler->release(compiler->begin());
      g_log.info() << "Compiler item: " << c->Print();
      string name = c->Name();
      parsers.push_back(new IncParser(auto_ptr<Rule>(c.release()), name, graphdir));
    }

    const List* start = NULL;
    const List* prev = NULL;
    string line;
    while (getline(cin, line)) {
      //line += "\n";
      for (size_t i=0; i < line.size(); ++i) {
        string c = string(1, line.at(i));
        g_log.info();
        g_log.info() << "* main: Inserting character '" << c << "'";
        const List* node = new List("", c);
        parsers.at(0).Insert(*node, prev);
        if (!start) { start = node; }
        Batch batch;
        parsers.front().ExtractChanges(batch);
        for (parser_mod_iter i = parsers.begin()+1; i != parsers.end(); ++i) {
          if (batch.IsEmpty()) {
            g_log.info() << "* main: IncParser returned no batch items.";
            break;
          } else {
            g_log.info() << "* main: IncParser returned batch; sending to parser.  Batch:" << batch.Print();
            i->ApplyBatch(batch);
            i->ExtractChanges(batch);
          }
        }
        prev = node;
      }
    }
    g_log.info() << "Clearing input";
    const List* i = start;
    while (i) {
      const List* j = i->right;
      delete i;
      i = j;
    }

  } catch (const SError& e) {
    g_log.error() << "Compilation framework error: " << e.what() << endl;
    return 1;
  } catch (const std::exception& e) {
    g_log.error() << "Unknown error: " << e.what() << endl;
    return 1;
  } catch (...) {
    g_log.error() << "Unknown error" << endl;
    return 1;
  }

  return 0;
}
