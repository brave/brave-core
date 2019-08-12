/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_start.h"

#include <string>
#include <libxml/tree.h>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_resource.h"

namespace brave_page_graph {

EdgeRequestStart::EdgeRequestStart(PageGraph* const graph,
    Node* const out_node, NodeResource* const in_node,
    const InspectorId request_id, const RequestType request_type) :
      EdgeRequest(graph, out_node, in_node, request_id, kRequestStatusStart),
      request_type_(request_type) {}

EdgeRequestStart::~EdgeRequestStart() {}

NodeResource* EdgeRequestStart::GetResourceNode() const {
  return static_cast<NodeResource*>(GetInNode());
}

Node* EdgeRequestStart::GetRequestingNode() const {
  return GetOutNode();
}

ItemName EdgeRequestStart::GetItemName() const {
  return "request start";
}

ItemDesc EdgeRequestStart::GetItemDesc() const {
  return EdgeRequest::GetItemDesc()
      + " [" + RequestTypeToString(request_type_) + "]";
}

void EdgeRequestStart::AddGraphMLAttributes(xmlDocPtr doc,
    xmlNodePtr parent_node) const {
  EdgeRequest::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefRequestType)
      ->AddValueNode(doc, parent_node, RequestTypeToString(request_type_));
}

bool EdgeRequestStart::IsEdgeRequestStart() const {
  return true;
}

}  // namespace brave_page_graph
