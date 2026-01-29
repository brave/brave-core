/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/request/edge_request.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/node_resource.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace brave_page_graph {

EdgeRequest::EdgeRequest(GraphItemContext* context,
                         GraphNode* out_node,
                         GraphNode* in_node,
                         const InspectorId request_id,
                         const FrameId& frame_id,
                         const RequestStatus request_status)
    : GraphEdge(context, out_node, in_node),
      request_id_(request_id),
      frame_id_(frame_id),
      request_status_(request_status) {}

EdgeRequest::~EdgeRequest() = default;

RequestURL EdgeRequest::GetRequestURL() const {
  return GetResourceNode()->GetURL();
}

void EdgeRequest::AddGraphMLAttributes(xmlDocPtr doc,
                                       xmlNodePtr parent_node) const {
  GraphEdge::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefRequestId)
      ->AddValueNode(doc, parent_node, request_id_);
  GraphMLAttrDefForType(kGraphMLAttrDefStatus)
      ->AddValueNode(doc, parent_node, RequestStatusToString(request_status_));
  GraphMLAttrDefForType(kGraphMLAttrDefEdgeFrameId)
      ->AddValueNode(doc, parent_node, frame_id_);

  // For these two attributes ("headers" and "size")
  // we create them in the generated XML / GraphML files
  // with stub values, which are then populated with their
  // true values in the `pagegraph-crawl` automation-side code.
  // This allows us to not need to reach into the network process
  // in blink / chromium (which we'd otherwise need to do to correctly
  // populate these values).
  GraphMLAttrDefForType(kGraphMLAttrDefHeaders)
      ->AddValueNode(doc, parent_node, blink::String(""));
  GraphMLAttrDefForType(kGraphMLAttrDefSize)
      ->AddValueNode(doc, parent_node, -1);
}

bool EdgeRequest::IsEdgeRequest() const {
  return true;
}

bool EdgeRequest::IsEdgeRequestStart() const {
  return false;
}

bool EdgeRequest::IsEdgeRequestResponse() const {
  return false;
}

}  // namespace brave_page_graph
