/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_resource.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

EdgeRequest::EdgeRequest(PageGraph* const graph, Node* const out_node,
    Node* const in_node, const InspectorId request_id,
    const RequestStatus status) :
      Edge(graph, out_node, in_node),
      request_id_(request_id),
      status_(status) {}

EdgeRequest::~EdgeRequest() {}

InspectorId EdgeRequest::GetRequestId() const {
  return request_id_;
}

RequestUrl EdgeRequest::GetRequestUrl() const {
  return GetResourceNode()->GetUrl();
}

GraphMLXMLList EdgeRequest::GraphMLAttributes() const {
  return GraphMLXMLList({
    GraphMLAttrDefForType(kGraphMLAttrDefEdgeType)
      ->ToValue("request"),
    GraphMLAttrDefForType(kGraphMLAttrDefRequestId)
      ->ToValue(request_id_),
    GraphMLAttrDefForType(kGraphMLAttrDefStatus)
      ->ToValue(RequestStatusToString(status_))
  });
}

}  // namespace brave_page_graph
