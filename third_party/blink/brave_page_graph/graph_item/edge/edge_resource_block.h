/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_RESOURCE_BLOCK_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_RESOURCE_BLOCK_H_

#include <string>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"

namespace brave_page_graph {

class NodeFilter;
class NodeShield;
class NodeResource;
class PageGraph;

class EdgeResourceBlock : public Edge {
friend class PageGraph;
 public:
  EdgeResourceBlock() = delete;
  ~EdgeResourceBlock() override;

  ItemName GetItemName() const override;

  bool IsEdgeResourceBlock() const override;

 protected:
  EdgeResourceBlock(PageGraph* const graph, NodeFilter* const out_node,
    NodeResource* const in_node);
  EdgeResourceBlock(PageGraph* const graph, NodeShield* const out_node,
    NodeResource* const in_node);
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeResourceBlock> {
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return edge.IsEdgeResourceBlock();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeResourceBlock>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_RESOURCE_BLOCK_H_
