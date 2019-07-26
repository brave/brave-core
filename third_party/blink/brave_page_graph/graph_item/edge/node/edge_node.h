/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_NODE_EDGE_NODE_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_NODE_EDGE_NODE_H_

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"

namespace brave_page_graph {

class NodeActor;
class NodeHTML;
class PageGraph;

class EdgeNode : public Edge {
friend class PageGraph;
 public:
  EdgeNode() = delete;
  ~EdgeNode() override;

  bool IsEdgeNode() const override;

  virtual bool IsEdgeNodeCreate() const;
  virtual bool IsEdgeNodeDelete() const;
  virtual bool IsEdgeNodeInsert() const;
  virtual bool IsEdgeNodeRemove() const;

 protected:
  EdgeNode(PageGraph* const graph, NodeActor* const out_node,
    NodeHTML* const in_node);
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeNode> {
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return edge.IsEdgeNode();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeNode>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_NODE_EDGE_NODE_H_
