/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/graph_item.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"

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
  return "n" + to_string(GetId());
}

GraphMLXML Node::GetGraphMLTag() const {
  stringstream builder;
  builder << "<node id=\"" + GetGraphMLId() + "\">";
  for (const GraphMLXML& elm : GetGraphMLAttributes()) {
    builder << elm;
  }
  builder << "</node>";
  return builder.str();
}

GraphMLXMLList Node::GetGraphMLAttributes() const {
  GraphMLXMLList attrs = GraphItem::GetGraphMLAttributes();
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefNodeType)
      ->ToValue(GetItemName()));
  return attrs;
}

bool Node::IsNode() const {
  return true;
}

bool Node::IsNodeActor() const {
  return false;
}

bool Node::IsNodeFilter() const {
  return false;
}

bool Node::IsNodeHTML() const {
  return false;
}

bool Node::IsNodeExtensions() const {
  return false;
}

bool Node::IsNodeResource() const {
  return false;
}

bool Node::IsNodeShield() const {
  return false;
}

bool Node::IsNodeShields() const {
  return false;
}

bool Node::IsNodeStorage() const {
  return false;
}

bool Node::IsNodeStorageRoot() const {
  return false;
}

bool Node::IsNodeWebAPI() const {
  return false;
}

}  // namespace brave_page_graph
