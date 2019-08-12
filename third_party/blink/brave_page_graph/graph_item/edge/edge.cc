/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"

#include <string>
#include <libxml/tree.h>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/graph_item.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

using ::std::string;
using ::std::to_string;

namespace brave_page_graph {

Edge::Edge(PageGraph* const graph, Node* const out_node, Node* const in_node) :
      GraphItem(graph),
      out_node_(out_node),
      in_node_(in_node) {}

Edge::Edge(Node* const out_node, Node* const in_node) :
      out_node_(out_node),
      in_node_(in_node) {}

Edge::~Edge() {}

GraphMLId Edge::GetGraphMLId() const {
  return "e" + to_string(GetId());
}

void Edge::AddGraphMLTag(xmlDocPtr doc, xmlNodePtr parent_node) const {
  xmlNodePtr new_node = xmlNewChild(parent_node, NULL, BAD_CAST "edge", NULL);
  xmlSetProp(new_node, BAD_CAST "id", BAD_CAST GetGraphMLId().c_str());
  xmlSetProp(new_node, BAD_CAST "source",
      BAD_CAST out_node_->GetGraphMLId().c_str());
  xmlSetProp(new_node, BAD_CAST "target",
      BAD_CAST in_node_->GetGraphMLId().c_str());
  AddGraphMLAttributes(doc, new_node);
}

void Edge::AddGraphMLAttributes(xmlDocPtr doc, xmlNodePtr parent_node) const {
  GraphItem::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefEdgeType)
      ->AddValueNode(doc, parent_node, GetItemName());
  GraphMLAttrDefForType(kGraphMLAttrDefPageGraphEdgeId)
      ->AddValueNode(doc, parent_node, GetId());
  if (graph_ != nullptr) {
    GraphMLAttrDefForType(kGraphMLAttrDefPageGraphEdgeTimestamp)
      ->AddValueNode(doc, parent_node, GetMicroSecSincePageStart());
  }
}

bool Edge::IsEdge() const {
  return true;
}

bool Edge::IsEdgeAttribute() const {
  return false;
}

bool Edge::IsEdgeCrossDOM() const {
  return false;
}

bool Edge::IsEdgeEventListener() const {
  return false;
}

bool Edge::IsEdgeEventListenerAction() const {
  return false;
}

bool Edge::IsEdgeExecute() const {
  return false;
}

bool Edge::IsEdgeFilter() const {
  return false;
}

bool Edge::IsEdgeHTML() const {
  return false;
}

bool Edge::IsEdgeNode() const {
  return false;
}

bool Edge::IsEdgeRequest() const {
  return false;
}

bool Edge::IsEdgeResourceBlock() const {
  return false;
}

bool Edge::IsEdgeShield() const {
  return false;
}

bool Edge::IsEdgeStorage() const {
  return false;
}

bool Edge::IsEdgeStorageBucket() const {
  return false;
}

bool Edge::IsEdgeTextChange() const {
  return false;
}

bool Edge::IsEdgeJS() const {
  return false;
}

}  // namespace brave_page_graph
