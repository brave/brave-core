/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_NODE_EDGE_NODE_DELETE_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_NODE_EDGE_NODE_DELETE_H_

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/node/edge_node.h"

namespace brave_page_graph {

class NodeHTML;
class NodeScript;
class PageGraph;

class EdgeNodeDelete final : public EdgeNode {
friend class PageGraph;
 public:
  EdgeNodeDelete() = delete;
  ~EdgeNodeDelete() override;

  ItemName GetItemName() const override;

  bool IsEdgeNodeDelete() const override;

 protected:
  EdgeNodeDelete(PageGraph* const graph, NodeScript* const out_node,
    NodeHTML* const in_node);
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeNodeDelete> {
  static bool AllowFrom(const brave_page_graph::EdgeNode& edge) {
    return edge.IsEdgeNodeDelete();
  }
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return IsA<brave_page_graph::EdgeNodeDelete>(
        DynamicTo<brave_page_graph::EdgeNode>(edge));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeNodeDelete>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_NODE_DELETE_H_
