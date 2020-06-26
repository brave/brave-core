/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

#include <string>
#include <vector>
#include <libxml/tree.h>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/graph_item.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"

using ::std::string;
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

void Node::AddGraphMLTag(xmlDocPtr doc, xmlNodePtr parent_node) const {
  xmlNodePtr new_node = xmlNewChild(parent_node, NULL, BAD_CAST "node", NULL);
  xmlSetProp(new_node, BAD_CAST "id", BAD_CAST GetGraphMLId().c_str());
  AddGraphMLAttributes(doc, new_node);
}

void Node::AddGraphMLAttributes(xmlDocPtr doc, xmlNodePtr parent_node) const {
  GraphItem::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefNodeType)
      ->AddValueNode(doc, parent_node, GetItemName());
  GraphMLAttrDefForType(kGraphMLAttrDefPageGraphNodeId)
      ->AddValueNode(doc, parent_node, GetId());
  GraphMLAttrDefForType(kGraphMLAttrDefPageGraphNodeTimestamp)
      ->AddValueNode(doc, parent_node, GetMicroSecSincePageStart());
}

bool Node::IsNode() const {
  return true;
}

bool Node::IsNodeActor() const {
  return false;
}

bool Node::IsNodeBinding() const {
  return false;
}

bool Node::IsNodeBindingEvent() const {
  return false;
}

bool Node::IsNodeExtensions() const {
  return false;
}

bool Node::IsNodeFilter() const {
  return false;
}

bool Node::IsNodeHTML() const {
  return false;
}

bool Node::IsNodeJS() const {
  return false;
}

bool Node::IsNodeRemoteFrame() const {
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

}  // namespace brave_page_graph
