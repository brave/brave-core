/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_start.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/graph_item.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_resource.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::to_string;

namespace brave_page_graph {

EdgeRequestStart::EdgeRequestStart(PageGraph* const graph,
    Node* const out_node, NodeResource* const in_node,
    const InspectorId request_id, const RequestType type) :
      EdgeRequest(graph, out_node, in_node, request_id, kRequestStatusStart),
      type_(type) {}

EdgeRequestStart::~EdgeRequestStart() {}

ItemName EdgeRequestStart::GetItemName() const {
  return "request start #" + to_string(id_);
}

RequestType EdgeRequestStart::GetRequestType() const {
  return type_;
}

NodeResource* EdgeRequestStart::GetResourceNode() const {
  return static_cast<NodeResource*>(in_node_);
}

RequestUrl EdgeRequestStart::GetRequestedUrl() const {
  return GetResourceNode()->GetUrl();
}

Node* EdgeRequestStart::GetRequestingNode() const {
  return out_node_;
}

ItemDesc EdgeRequestStart::GetDescBody() const {
  return GetItemName() + " (" + RequestTypeToString(type_) + ")";
}

GraphMLXMLList EdgeRequestStart::GraphMLAttributes() const {
  GraphMLXMLList attrs = EdgeRequest::GraphMLAttributes();
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefRequestType)
      ->ToValue(RequestTypeToString(type_)));
  return attrs;
}

}  // namespace brave_page_graph
