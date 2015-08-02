// Copyright (C) 2015 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#include "Keyword.h"

#include "statik/Batch.h"
#include "statik/IncParser.h"
#include "statik/Keyword.h"
#include "statik/List.h"
#include "statik/Rule.h"
#include "statik/SError.h"
#include "statik/STree.h"
using statik::Batch;
using statik::IncParser;
using statik::KEYWORD;
using statik::List;
using statik::Rule;

#include <memory>
#include <string>
using std::auto_ptr;
using std::string;

using namespace statik_test;

void Keyword::run() {
  g_log.info();
  g_log.info() << "Blank keyword";
  {
    bool p = false;
    string msg;
    try {
      auto_ptr<Rule> r(KEYWORD(""));
      IncParser ip(r, "Keyword test 1");
    } catch (const statik::SError& e) {
      p = true;
      msg = e.what();
    }
    test(p, "Empty keyword not allowed: " + msg);
  }

  g_log.info();
  g_log.info() << "Single-character keyword";
  {
    auto_ptr<Rule> r(KEYWORD("a"));
    IncParser ip(r, "Keyword test 2");
    const statik::STree& root = ip.GetRoot();

    Batch empty_batch;
    {
      Batch out_batch;
      List cx("cx", "x");
      ip.Insert(cx, NULL);
      test(root.GetState().IsBad(), "x");
      ip.ExtractChanges(out_batch);
      test(out_batch, empty_batch, "no output");
      /*
      // Even though it's bad, it contributes its output node
      // xxx -- not anymore!
      if (test(out_batch.Size(), (size_t)1, "output")) {
        const Batch::BatchItem& item = *out_batch.begin();
        if (test(item.node, "batch insert node")) {
          test(item.node->name, string("a"), "batch insert node name");
          test(item.node->value, string(""), "batch insert node value");
        }
        test(item.op, Batch::OP_INSERT, "batch insert op");
        test(item.pos, (const List*)NULL, "batch insert pos");
      }
      */
      out_batch.Clear();

      ip.Delete(cx);
      test(root.IsClear(), "clear");
      test(out_batch, empty_batch, "no output");
    }

    {
      Batch out_batch;
      List ca("ca", "a");
      ip.Insert(ca, NULL);
      test(root.GetState().IsDone(), "a");
      ip.ExtractChanges(out_batch);
      {
        Batch expected_batch;
        List xca("a", "");
        expected_batch.Insert(xca, NULL);
        test(out_batch, expected_batch, "yay output");
      }
      out_batch.Clear();

      List cb("cb", "b");
      ip.Insert(cb, &ca);
      test(root.GetState().IsComplete(), "ab");
      ip.ExtractChanges(out_batch);
      test(out_batch, empty_batch, "no output");

      List cc("cc", "c");
      ip.Insert(cc, &cb);
      test(root.GetState().IsBad(), "abc");
      ip.ExtractChanges(out_batch);
      test(out_batch, empty_batch, "no output");

      ip.Delete(cb);
      test(root.GetState().IsComplete(), "ac");
      ip.ExtractChanges(out_batch);
      test(out_batch, empty_batch, "no output");

      ip.Delete(ca);
      test(root.GetState().IsBad(), "c");
      g_log.info() << "ok3";
      ip.ExtractChanges(out_batch);
      g_log.info() << "ok4";
      test(out_batch, empty_batch, "no output");
      g_log.info() << "ok5";

      List ca2("2", "a");
      ip.Insert(ca2, NULL);
      test(root.GetState().IsComplete(), "ac");
      ip.ExtractChanges(out_batch);
      test(out_batch, empty_batch, "no output");

      ip.Delete(cc);
      test(root.GetState().IsDone(), "a");
      ip.ExtractChanges(out_batch);
      test(out_batch, empty_batch, "no output");

      ip.Delete(ca2);
      test(root.IsClear());
      ip.ExtractChanges(out_batch);
      test(out_batch, empty_batch, "no output");
    }

    // Batch updates
    g_log.info();
    g_log.info() << "Batch updates";
    {
      Batch b;
      List ca("ca", "a");
      List cb("cb", "b");

      b.Insert(ca, NULL);
      ip.ApplyBatch(b);
      test(root.GetState().IsDone(), "a");

      b.Clear();
      b.Delete(ca);
      ip.ApplyBatch(b);
      test(root.IsClear(), "clear");

      b.Clear();
      b.Insert(ca, NULL);
      b.Insert(cb, &ca);
      ip.ApplyBatch(b);
      test(root.GetState().IsComplete(), "ab");

      b.Clear();
      b.Delete(ca);
      b.Delete(cb);
      ip.ApplyBatch(b);
      test(root.IsClear(), "clear");
    }
  }
}
