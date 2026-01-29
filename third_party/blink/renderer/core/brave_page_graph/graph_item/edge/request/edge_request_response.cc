/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/request/edge_request_response.h"

#include "base/strings/string_number_conversions.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/node_resource.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"

namespace brave_page_graph {

EdgeRequestResponse::EdgeRequestResponse(GraphItemContext* context,
                                         NodeResource* out_node,
                                         GraphNode* in_node,
                                         const InspectorId request_id,
                                         const FrameId& frame_id,
                                         const RequestStatus request_status)
    : EdgeRequest(context,
                  out_node,
                  in_node,
                  request_id,
                  frame_id,
                  request_status) {}

EdgeRequestResponse::~EdgeRequestResponse() = default;

NodeResource* EdgeRequestResponse::GetResourceNode() const {
  return static_cast<NodeResource*>(GetOutNode());
}

GraphNode* EdgeRequestResponse::GetRequestingNode() const {
  return GetInNode();
}

bool EdgeRequestResponse::IsEdgeRequestResponse() const {
  return true;
}

bool EdgeRequestResponse::IsEdgeRequestComplete() const {
  return false;
}

bool EdgeRequestResponse::IsEdgeRequestRedirect() const {
  return false;
}

bool EdgeRequestResponse::IsEdgeRequestError() const {
  return false;
}

}  // namespace brave_page_graph
