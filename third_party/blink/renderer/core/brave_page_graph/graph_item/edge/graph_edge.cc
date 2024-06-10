/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/graph_edge.h"

#include <string>

#include "base/strings/string_number_conversions.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/graph_item.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/graph_node.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"

namespace brave_page_graph {

GraphEdge::GraphEdge(GraphItemContext* context,
                     GraphNode* out_node,
                     GraphNode* in_node)
    : GraphItem(context), out_node_(out_node), in_node_(in_node) {
  DCHECK(out_node_);
  DCHECK(in_node_);
}

GraphEdge::~GraphEdge() = default;

GraphMLId GraphEdge::GetGraphMLId() const {
  return "e" + base::NumberToString(GetId());
}

void GraphEdge::AddGraphMLTag(xmlDocPtr doc, xmlNodePtr parent_node) const {
  xmlNodePtr new_node =
      xmlNewChild(parent_node, nullptr, BAD_CAST "edge", nullptr);
  xmlSetProp(new_node, BAD_CAST "id", BAD_CAST GetGraphMLId().c_str());
  xmlSetProp(new_node, BAD_CAST "source",
             BAD_CAST out_node_->GetGraphMLId().c_str());
  xmlSetProp(new_node, BAD_CAST "target",
             BAD_CAST in_node_->GetGraphMLId().c_str());
  AddGraphMLAttributes(doc, new_node);
}

void GraphEdge::AddGraphMLAttributes(xmlDocPtr doc,
                                     xmlNodePtr parent_node) const {
  GraphItem::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefEdgeType)
      ->AddValueNode(doc, parent_node, GetItemName());
  GraphMLAttrDefForType(kGraphMLAttrDefPageGraphEdgeId)
      ->AddValueNode(doc, parent_node, GetId());
  GraphMLAttrDefForType(kGraphMLAttrDefPageGraphEdgeTimestamp)
      ->AddValueNode(doc, parent_node,
                     GetTimeDeltaSincePageStart().InMilliseconds());
}

bool GraphEdge::IsEdge() const {
  return true;
}

bool GraphEdge::IsEdgeAttribute() const {
  return false;
}

bool GraphEdge::IsEdgeBinding() const {
  return false;
}

bool GraphEdge::IsEdgeBindingEvent() const {
  return false;
}

bool GraphEdge::IsEdgeCrossDOM() const {
  return false;
}

bool GraphEdge::IsEdgeDocument() const {
  return false;
}

bool GraphEdge::IsEdgeEventListener() const {
  return false;
}

bool GraphEdge::IsEdgeEventListenerAction() const {
  return false;
}

bool GraphEdge::IsEdgeExecute() const {
  return false;
}

bool GraphEdge::IsEdgeFilter() const {
  return false;
}

bool GraphEdge::IsEdgeJS() const {
  return false;
}

bool GraphEdge::IsEdgeNode() const {
  return false;
}

bool GraphEdge::IsEdgeRequest() const {
  return false;
}

bool GraphEdge::IsEdgeResourceBlock() const {
  return false;
}

bool GraphEdge::IsEdgeShield() const {
  return false;
}

bool GraphEdge::IsEdgeStorage() const {
  return false;
}

bool GraphEdge::IsEdgeStorageBucket() const {
  return false;
}

bool GraphEdge::IsEdgeTextChange() const {
  return false;
}

bool GraphEdge::IsEdgeStructure() const {
  return false;
}

}  // namespace brave_page_graph
