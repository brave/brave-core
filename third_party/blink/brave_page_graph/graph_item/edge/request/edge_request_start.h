/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_START_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_START_H_

#include <string>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request.h"

namespace brave_page_graph {

class Node;
class NodeResource;
class PageGraph;

class EdgeRequestStart final : public EdgeRequest {
friend class PageGraph;
 public:
  EdgeRequestStart() = delete;
  ~EdgeRequestStart() override;

  RequestType GetRequestType() const { return request_type_; }

  NodeResource* GetResourceNode() const override;
  Node* GetRequestingNode() const override;

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  GraphMLXMLList GetGraphMLAttributes() const override;

  bool IsEdgeRequestStart() const override;

 protected:
  EdgeRequestStart(PageGraph* const graph, Node* const out_node,
    NodeResource* const in_node, const InspectorId request_id,
    const RequestType request_type);

 private:
  const RequestType request_type_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeRequestStart> {
  static bool AllowFrom(const brave_page_graph::EdgeRequest& request_edge) {
    return request_edge.IsEdgeRequestStart();
  }
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return IsA<brave_page_graph::EdgeRequestStart>(
        DynamicTo<brave_page_graph::EdgeRequest>(edge));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeRequestStart>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_REQUEST_EDGE_REQUEST_START_H_
