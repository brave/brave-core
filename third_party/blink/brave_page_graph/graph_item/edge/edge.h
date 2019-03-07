/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_H_

#include "brave/third_party/blink/brave_page_graph/graph_item/graph_item.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class Node;
class PageGraph;

class Edge : public GraphItem {
friend class Node;
friend class PageGraph;
 public:
  Edge() = delete;
  GraphMLId GetGraphMLId() const override;
  GraphMLXML GetGraphMLTag() const override;

 protected:
  Edge(PageGraph* const graph, Node* const out_node, Node* const in_node);

  // This constructor is used by the GraphML generation pipeline, where
  // we want to maintain const correctness and commit to not modifying
  // the out node.  This should never be called otherwise.
  Edge(PageGraph* const graph, const Node* const out_node, Node* const in_node);

  ItemDesc GetDescPrefix() const override;
  ItemDesc GetDescSuffix() const override;

  // These pointers are not owning, the PageGraph instance owns them.
  Node* const out_node_;
  Node* const in_node_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_H_
