/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include <ostream>
#include <sstream>
#include <string>
#include <vector>
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/graph_item.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::string;
using ::std::stringstream;
using ::std::to_string;
using ::std::vector;

namespace brave_page_graph {

Node::Node(PageGraph* const graph) :
      GraphItem(graph) {}

Node::~Node() {}

void Node::AddInEdge(const Edge* const in_edge) {
  in_edges_.push_back(in_edge);
}

void Node::AddOutEdge(const Edge* const out_edge) {
  out_edges_.push_back(out_edge);
}

GraphMLId Node::GetGraphMLId() const {
  return "n" + to_string(id_);
}

GraphMLXML Node::GetGraphMLTag() const {
  stringstream builder;
  builder << "<node id=\"" + GetGraphMLId() + "\">";
  for (const GraphMLXML& elm : GraphMLAttributes()) {
    builder << elm;
  }
  builder << "</node>";
  return builder.str();
}

ItemDesc Node::GetDescPrefix() const {
  stringstream string_builder;
  for (const Edge* elm : in_edges_) {
    string_builder << elm->GetItemName() << " -> \n";
  }
  string_builder << "  ";
  return string_builder.str();
}

ItemDesc Node::GetDescSuffix() const {
  stringstream string_builder;
  string_builder << "\n";
  for (const Edge* elm : out_edges_) {
    string_builder << "     -> " << elm->GetItemName() << "\n";
  }
  return string_builder.str();
}

}  // namespace brave_page_graph
