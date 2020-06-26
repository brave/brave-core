/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EXECUTE_EDGE_EXECUTE_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EXECUTE_EDGE_EXECUTE_H_

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"

namespace brave_page_graph {

class NodeExtensions;
class NodeHTMLElement;
class NodeScript;
class PageGraph;

class EdgeExecute : public Edge {
friend class PageGraph;
 public:
  EdgeExecute() = delete;
  ~EdgeExecute() override;

  ItemName GetItemName() const override;

  bool IsEdgeExecute() const override;

  virtual bool IsEdgeExecuteAttr() const;

 protected:
  EdgeExecute(PageGraph* const graph, NodeHTMLElement* const out_node,
    NodeScript* const in_node);
  EdgeExecute(PageGraph* const graph, NodeExtensions* const out_node,
    NodeScript* const in_node);
  // Used by imported module scripts and eval
  EdgeExecute(PageGraph* const graph, NodeScript* const out_node,
    NodeScript* const in_node);
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeExecute> {
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return edge.IsEdgeExecute();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeExecute>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EXECUTE_EDGE_EXECUTE_H_
