/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_NODE_EDGE_NODE_CREATE_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_NODE_EDGE_NODE_CREATE_H_

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/node/edge_node.h"

namespace brave_page_graph {

class NodeActor;
class NodeHTML;
class PageGraph;

class EdgeNodeCreate final : public EdgeNode {
friend class PageGraph;
 public:
  EdgeNodeCreate() = delete;
  ~EdgeNodeCreate() override;

  ItemName GetItemName() const override;

  bool IsEdgeNodeCreate() const override;

 protected:
  EdgeNodeCreate(PageGraph* const graph, NodeActor* const out_node,
    NodeHTML* const in_node);
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeNodeCreate> {
  static bool AllowFrom(const brave_page_graph::EdgeNode& edge) {
    return edge.IsEdgeNodeCreate();
  }
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return IsA<brave_page_graph::EdgeNodeCreate>(
        DynamicTo<brave_page_graph::EdgeNode>(edge));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeNodeCreate>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_NODE_EDGE_NODE_CREATE_H_
