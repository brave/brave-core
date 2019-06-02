/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_response.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_resource.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

EdgeRequestResponse::EdgeRequestResponse(PageGraph* const graph,
    NodeResource* const out_node, Node* const in_node,
    const InspectorId request_id, const RequestStatus status) :
      EdgeRequest(graph, out_node, in_node, request_id, status) {}

EdgeRequestResponse::~EdgeRequestResponse() {}

NodeResource* EdgeRequestResponse::GetResourceNode() const {
  return static_cast<NodeResource*>(out_node_);
}

Node* EdgeRequestResponse::GetRequestingNode() const {
  return in_node_;
}

}  // namespace brave_page_graph

