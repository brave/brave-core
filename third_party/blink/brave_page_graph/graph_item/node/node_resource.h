/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_RESOURCE_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_RESOURCE_H_

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class EdgeRequestStart;
class EdgeRequestResponse;
class EdgeResourceBlock;
class PageGraph;

class NodeResource : public Node {
friend class PageGraph;
 public:
  NodeResource() = delete;
  ~NodeResource() override;
  ItemName GetItemName() const override;
  RequestUrl GetUrl() const;

  // Prevent non-sensical edges from ever pointing to a resource node.
  void AddInEdge(const EdgeRequestStart* const in_edge);
  void AddOutEdge(const EdgeRequestResponse* const out_edge);
  void AddInEdge(const EdgeResourceBlock* const in_edge);
  void AddInEdge(const Edge* const in_edge) = delete;
  void AddOutEdge(const Edge* const out_edge) = delete;

  bool IsNodeActor() const override { return false; }

 protected:
  NodeResource(PageGraph* const graph, const RequestUrl url);
  ItemDesc GetDescBody() const override;
  GraphMLXMLList GraphMLAttributes() const override;

  const RequestUrl url_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_RESOURCE_H_
