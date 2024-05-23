/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_RESPONSE_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_RESPONSE_H_

#include <string>

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/request/edge_request.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/utilities/response_metadata.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace brave_page_graph {

class GraphNode;
class NodeResource;

class EdgeRequestResponse : public EdgeRequest {
 public:
  EdgeRequestResponse(GraphItemContext* context,
                      NodeResource* out_node,
                      GraphNode* in_node,
                      const InspectorId request_id,
                      const FrameId& frame_id,
                      const RequestStatus request_status,
                      const ResponseMetadata& metadata);

  ~EdgeRequestResponse() override;

  const std::string& GetResponseHeaderString() const {
    return response_header_string_;
  }
  NodeResource* GetResourceNode() const override;
  GraphNode* GetRequestingNode() const override;

  void AddGraphMLAttributes(xmlDocPtr doc,
                            xmlNodePtr parent_node) const override;

  bool IsEdgeRequestResponse() const override;

  virtual bool IsEdgeRequestComplete() const;
  virtual bool IsEdgeRequestRedirect() const;
  virtual bool IsEdgeRequestError() const;

 private:
  const std::string response_header_string_;
  const int64_t response_data_length_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeRequestResponse> {
  static bool AllowFrom(const brave_page_graph::EdgeRequest& request_edge) {
    return request_edge.IsEdgeRequestResponse();
  }
  static bool AllowFrom(const brave_page_graph::GraphEdge& edge) {
    return IsA<brave_page_graph::EdgeRequestResponse>(
        DynamicTo<brave_page_graph::EdgeRequest>(edge));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeRequestResponse>(
        DynamicTo<brave_page_graph::GraphEdge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_RESPONSE_H_
