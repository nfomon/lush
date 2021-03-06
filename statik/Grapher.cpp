// Copyright (C) 2014 Michael Biggs.  See the COPYING file at the top-level
// directory of this distribution and at http://shok.io/code/copyright.html

#include "Grapher.h"

#include "List.h"
#include "Rule.h"
#include "SError.h"
#include "SLog.h"
#include "STree.h"

#include "util/Graphviz.h"
#include "util/Util.h"
using Util::dotVar;
using Util::safeLabelStr;

#include <boost/lexical_cast.hpp>
using boost::lexical_cast;

#include <fstream>
#include <iostream>
#include <string>
using std::ofstream;
using std::string;

using namespace statik;

/* public */

Grapher::Grapher(const string& out_dir, const string& base_filename)
  : m_out_dir(out_dir),
    m_base_filename(base_filename),
    m_img_count(0),
    m_isDirty(false) {
  Clear();
}

void Grapher::AddMachine(const string& context, const Rule& machineRoot) {
  m_graph += "subgraph cluster_" + dotVar(&machineRoot, context) + " {\n";
  m_graph += "node [shape=box];\n";
  m_graph += "graph [style=\"filled\", fillcolor=\"#f0f0f0\"];\n";
  m_graph += machineRoot.DrawNode(context);
  m_graph += "}\n";
  m_isDirty = true;
}

void Grapher::AddOrderList(const string& context, const OrderList& orderList, const List& start) {
  m_graph += "subgraph cluster_" + dotVar(&orderList, context) + " {\n";
  m_graph += "node [shape=box];\n";
  m_graph += "graph [style=\"filled\", fillcolor=\"#f0e0d0\"];\n";
  m_graph += orderList.Draw(context, start);
  m_graph += "}\n";
  m_isDirty = true;
}

void Grapher::AddIList(const string& context, const List& inode, const string& label) {
  m_graph += "subgraph cluster_" + context + dotVar(&inode, context) + " {\n";
  if (!label.empty()) {
    m_graph += "label=\"" + label + "\";\n";
    m_graph += "fontsize=12.0;\n";
  }
  m_graph += inode.DrawNode(context);
  m_graph += "}\n";
  m_isDirty = true;
}

void Grapher::AddSTree(const string& context, const STree& snode, const string& label, const STree* initiator) {
  m_graph += "subgraph cluster_" + context + dotVar(&snode, context) + " {\n";
  if (!label.empty()) {
    m_graph += "label=\"" + label + "\";\n";
    m_graph += "fontsize=12.0;\n";
  }
  // include the root node:
  m_graph += snode.DrawNode(context, initiator);
  /*
  // or alternatively, skip the root node:
  for (STree::child_iter i = snode.children.begin(); i != snode.children.end(); ++i) {
    m_graph += (*i)->DrawNode(context, initiator);
  }
  */
  m_graph += "}\n";
  m_isDirty = true;
}

void Grapher::AddIListeners(const string& context, const IncParser& incParser, const List& inode) {
  IncParser::listener_set ls = incParser.GetListeners(inode);
  for (IncParser::listener_iter i = ls.begin(); i != ls.end(); ++i) {
    m_graph += dotVar(&inode, context) + " -> " + dotVar(*i, context) + " [constraint=false, weight=0, style=dashed, arrowsize=0.5, color=\"#3333cc\"];\n";
  }
  if (inode.right) {
    AddIListeners(context, incParser, *inode.right);
  }
  m_isDirty = true;
}

void Grapher::AddOBatch(const string& context, const Batch& batch, const string& label) {
  if (batch.IsEmpty()) {
    return;
  }
  const List& start = *batch.begin()->node;
  m_graph += "subgraph cluster_" + context + dotVar(&start, context) + " {\n";
  if (!label.empty()) {
    m_graph += "label=\"" + label + "\";\n";
    m_graph += "fontsize=12.0;\n";
  }

  for (Batch::batch_iter i = batch.begin(); i != batch.end(); ++i) {
    string color = "#770000";
    switch (i->op) {
    case Batch::OP_INSERT:
      color = "#00bb44";
      break;
    case Batch::OP_UPDATE:
      color = "#2222cc";
      break;
    case Batch::OP_DELETE:
      color = "#cc2222";
      break;
    default:
      throw SError("Cannot add batch to Grapher: unknown operation");
    }
    if (Batch::OP_DELETE == i->op) {
      m_graph += dotVar(i->node, context) + " [label=\"(deleted)\", style=\"filled\", fillcolor=\"#dddddd\", fontsize=12.0, color=\"" + color + "\", penwidth=4.0];\n";
      if (i+1 != batch.end()) {
        const List& next = *(i+1)->node;
        m_graph += dotVar(i->node, context) + " -> " + dotVar(&next, context) + ";\n";
      }
    } else {
      const List& node = *i->node;
      m_graph += dotVar(&node, context) + " [label=\"" + Util::safeLabelStr(node.name + (node.value.empty() ? "" : (node.name.empty() ? "" : ":") + node.value)) + "\", style=\"filled\", fillcolor=\"#dddddd\", fontsize=12.0, color=\"" + color + "\", penwidth=4.0];\n";
      if (i+1 != batch.end()) {
        const List& next = *(i+1)->node;
        m_graph += dotVar(&node, context) + " -> " + dotVar(&next, context) + ";\n";
      }
    }
  }
  m_graph += "}\n";
  m_isDirty = true;
}

void Grapher::Signal(const string& context, const void* x, bool isUpdate) {
  string color = "#dd3300";
  if (isUpdate) {
    color = "#003399";
  }
  m_graph += dotVar(x, context) + " [color=\"" + color + "\", penwidth=3.0];\n";
  m_isDirty = true;
}

void Grapher::Save() {
  if (!m_isDirty) {
    g_log.info() << "GGG Grapher: Not saving graph; not dirty";
    return;
  }
  m_graph += "}\n";
  string outfile(m_out_dir + "/" + m_base_filename + Util::pad_str(lexical_cast<string>(m_img_count), '0', 3) + ".dot");
  g_log.debug();
  g_log.debug() << "GGG Grapher: Saving graph " << outfile;
  g_log.debug();
  ofstream out(outfile.c_str());
  if (!out) {
    throw SError("Failed to open output graph file " + outfile);
  }
  out << m_graph;
  out.close();
  ++m_img_count;
}

void Grapher::Clear() {
  m_isDirty = false;
  m_graph = "digraph {\n";
  m_graph += "graph [compound=true, splines=line, dpi=65];\n";
}
