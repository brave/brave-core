/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/graph_node.h"

#include "base/strings/string_number_conversions.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/graph_edge.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"

namespace brave_page_graph {

GraphNode::GraphNode(GraphItemContext* context) : GraphItem(context) {}

GraphNode::~GraphNode() = default;

void GraphNode::AddInEdge(const GraphEdge* in_edge) {
  in_edges_.push_back(in_edge);
}

void GraphNode::AddOutEdge(const GraphEdge* out_edge) {
  out_edges_.push_back(out_edge);
}

GraphMLId GraphNode::GetGraphMLId() const {
  return "n" + base::NumberToString(GetId());
}

void GraphNode::AddGraphMLTag(xmlDocPtr doc, xmlNodePtr parent_node) const {
  xmlNodePtr new_node =
      xmlNewChild(parent_node, nullptr, BAD_CAST "node", nullptr);
  xmlSetProp(new_node, BAD_CAST "id", BAD_CAST GetGraphMLId().c_str());
  AddGraphMLAttributes(doc, new_node);
}

void GraphNode::AddGraphMLAttributes(xmlDocPtr doc,
                                     xmlNodePtr parent_node) const {
  GraphItem::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefNodeType)
      ->AddValueNode(doc, parent_node, GetItemName());
  GraphMLAttrDefForType(kGraphMLAttrDefPageGraphNodeId)
      ->AddValueNode(doc, parent_node, GetId());
  GraphMLAttrDefForType(kGraphMLAttrDefPageGraphNodeTimestamp)
      ->AddValueNode(doc, parent_node,
                     GetTimeDeltaSincePageStart().InMilliseconds());
}

bool GraphNode::IsNode() const {
  return true;
}

bool GraphNode::IsNodeActor() const {
  return false;
}

bool GraphNode::IsNodeBinding() const {
  return false;
}

bool GraphNode::IsNodeBindingEvent() const {
  return false;
}

bool GraphNode::IsNodeExtensions() const {
  return false;
}

bool GraphNode::IsNodeFilter() const {
  return false;
}

bool GraphNode::IsNodeHTML() const {
  return false;
}

bool GraphNode::IsNodeJS() const {
  return false;
}

bool GraphNode::IsNodeRemoteFrame() const {
  return false;
}

bool GraphNode::IsNodeResource() const {
  return false;
}

bool GraphNode::IsNodeShield() const {
  return false;
}

bool GraphNode::IsNodeShields() const {
  return false;
}

bool GraphNode::IsNodeStorage() const {
  return false;
}

bool GraphNode::IsNodeStorageRoot() const {
  return false;
}

}  // namespace brave_page_graph
