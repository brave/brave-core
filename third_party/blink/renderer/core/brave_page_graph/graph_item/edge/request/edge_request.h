/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_H_

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/graph_edge.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace brave_page_graph {

class GraphNode;
class NodeResource;

class EdgeRequest : public GraphEdge {
 public:
  EdgeRequest(GraphItemContext* context,
              GraphNode* out_node,
              GraphNode* in_node,
              const InspectorId request_id,
              const FrameId& frame_id,
              const RequestStatus request_status);
  ~EdgeRequest() override;

  InspectorId GetRequestId() const { return request_id_; }
  RequestStatus GetRequestStatus() const { return request_status_; }
  RequestURL GetRequestURL() const;

  // This is just a more-semantically meaningful alias for which
  // node is the requestor and which is the resource, which will differ
  // depending on the request status (i.e. initiation, response or error).
  virtual NodeResource* GetResourceNode() const = 0;
  virtual GraphNode* GetRequestingNode() const = 0;

  void AddGraphMLAttributes(xmlDocPtr doc,
                            xmlNodePtr parent_node) const override;

  bool IsEdgeRequest() const override;

  virtual bool IsEdgeRequestStart() const;
  virtual bool IsEdgeRequestResponse() const;

 private:
  const InspectorId request_id_;
  const FrameId frame_id_;
  const RequestStatus request_status_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeRequest> {
  static bool AllowFrom(const brave_page_graph::GraphEdge& edge) {
    return edge.IsEdgeRequest();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeRequest>(
        DynamicTo<brave_page_graph::GraphEdge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_H_
