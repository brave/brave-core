/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_H_

#include "brave/third_party/blink/brave_page_graph/graph_item/graph_item.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class Edge;
class PageGraph;

class Node : public GraphItem {
friend class Edge;
friend class PageGraph;
 public:
  Node() = delete;
  ~Node() override;
  void AddInEdge(const Edge* const in_edge);
  void AddOutEdge(const Edge* const out_edge);

  GraphMLId GetGraphMLId() const override;
  GraphMLXML GetGraphMLTag() const override;

 protected:
  Node(PageGraph* const graph);
  ItemDesc GetDescPrefix() const override;
  ItemDesc GetDescSuffix() const override;

  // Reminder to self:
  //   out_edge -> node -> in_edge
  // These vectors do not own their references.  All nodes in the entire
  // graph are owned by the PageGraph instance.
  EdgeList in_edges_;
  EdgeList out_edges_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_H_
