/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/request/edge_request_start.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/graph_node.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/node_resource.h"

namespace brave_page_graph {

EdgeRequestStart::EdgeRequestStart(GraphItemContext* context,
                                   GraphNode* out_node,
                                   NodeResource* in_node,
                                   const InspectorId request_id,
                                   const std::string& resource_type)
    : EdgeRequest(context, out_node, in_node, request_id, kRequestStatusStart),
      resource_type_(resource_type) {}

EdgeRequestStart::~EdgeRequestStart() = default;

NodeResource* EdgeRequestStart::GetResourceNode() const {
  return static_cast<NodeResource*>(GetInNode());
}

GraphNode* EdgeRequestStart::GetRequestingNode() const {
  return GetOutNode();
}

ItemName EdgeRequestStart::GetItemName() const {
  return "request start";
}

ItemDesc EdgeRequestStart::GetItemDesc() const {
  return EdgeRequest::GetItemDesc() + " [" + resource_type_ + "]";
}

void EdgeRequestStart::AddGraphMLAttributes(xmlDocPtr doc,
                                            xmlNodePtr parent_node) const {
  EdgeRequest::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefResourceType)
      ->AddValueNode(doc, parent_node, resource_type_);
}

bool EdgeRequestStart::IsEdgeRequestStart() const {
  return true;
}

}  // namespace brave_page_graph
