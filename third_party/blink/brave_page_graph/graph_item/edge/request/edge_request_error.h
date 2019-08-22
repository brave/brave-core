/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_ERROR_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_ERROR_H_

#include <string>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"
#include "brave/third_party/blink/brave_page_graph/utilities/response_metadata.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_response.h"

namespace brave_page_graph {

class Node;
class NodeResource;
class PageGraph;

class EdgeRequestError final : public EdgeRequestResponse {
friend class PageGraph;
 public:
  EdgeRequestError() = delete;
  ~EdgeRequestError() override;

  ItemName GetItemName() const override;

  bool IsEdgeRequestError() const override;

 protected:
  EdgeRequestError(PageGraph* const graph, NodeResource* const out_node,
    Node* const in_node, const InspectorId request_id,
    const ResponseMetadata& metadata);
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeRequestError> {
  static bool AllowFrom(
      const brave_page_graph::EdgeRequestResponse& request_response_edge) {
    return request_response_edge.IsEdgeRequestError();
  }
  static bool AllowFrom(const brave_page_graph::EdgeRequest& request_edge) {
    return IsA<brave_page_graph::EdgeRequestError>(
        DynamicTo<brave_page_graph::EdgeRequestResponse>(request_edge));
  }
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return IsA<brave_page_graph::EdgeRequestError>(
        DynamicTo<brave_page_graph::EdgeRequest>(edge));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeRequestError>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_ERROR_H_
