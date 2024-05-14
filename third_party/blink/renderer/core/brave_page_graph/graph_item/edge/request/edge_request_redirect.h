/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_REDIRECT_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_REDIRECT_H_

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/request/edge_request_response.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/utilities/response_metadata.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace brave_page_graph {

class NodeResource;

class EdgeRequestRedirect final : public EdgeRequestResponse {
 public:
  EdgeRequestRedirect(GraphItemContext* context,
                      NodeResource* out_node,
                      NodeResource* in_node,
                      const InspectorId request_id,
                      const FrameId& frame_id,
                      const ResponseMetadata& metadata);
  ~EdgeRequestRedirect() override;

  ItemName GetItemName() const override;

  bool IsEdgeRequestRedirect() const override;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeRequestRedirect> {
  static bool AllowFrom(
      const brave_page_graph::EdgeRequestResponse& request_response_edge) {
    return request_response_edge.IsEdgeRequestRedirect();
  }
  static bool AllowFrom(const brave_page_graph::EdgeRequest& request_edge) {
    return IsA<brave_page_graph::EdgeRequestRedirect>(
        DynamicTo<brave_page_graph::EdgeRequestResponse>(request_edge));
  }
  static bool AllowFrom(const brave_page_graph::GraphEdge& edge) {
    return IsA<brave_page_graph::EdgeRequestRedirect>(
        DynamicTo<brave_page_graph::EdgeRequest>(edge));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeRequestRedirect>(
        DynamicTo<brave_page_graph::GraphEdge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_REDIRECT_H_
