/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_H_

#include <libxml/tree.h>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/graph_item.h"

namespace brave_page_graph {

class Node;
class PageGraph;

class Edge : public GraphItem {
friend class PageGraph;
 public:
  Edge() = delete;
  ~Edge() override;

  Node* GetOutNode() const { return out_node_; }
  Node* GetInNode() const { return in_node_; }

  GraphMLId GetGraphMLId() const override;
  void AddGraphMLTag(xmlDocPtr doc, xmlNodePtr parent_node) const override;
  void AddGraphMLAttributes(xmlDocPtr doc, xmlNodePtr parent_node)
      const override;

  bool IsEdge() const override;

  virtual bool IsEdgeAttribute() const;
  virtual bool IsEdgeCrossDOM() const;
  virtual bool IsEdgeEventListener() const;
  virtual bool IsEdgeEventListenerAction() const;
  virtual bool IsEdgeExecute() const;
  virtual bool IsEdgeFilter() const;
  virtual bool IsEdgeHTML() const;
  virtual bool IsEdgeNode() const;
  virtual bool IsEdgeRequest() const;
  virtual bool IsEdgeResourceBlock() const;
  virtual bool IsEdgeShield() const;
  virtual bool IsEdgeStorage() const;
  virtual bool IsEdgeStorageBucket() const;
  virtual bool IsEdgeTextChange() const;
  virtual bool IsEdgeJS() const;

 protected:
  Edge(PageGraph* const graph, Node* const out_node, Node* const in_node);
  // This constructor is used by the GraphML generation pipeline, where
  // we want to maintain const correctness and commit to not modifying
  // the out node.  This should never be called otherwise.
  Edge(PageGraph* const graph, const Node* const out_node, Node* const in_node);
  // For use ONLY with edges generated ad-hoc during GraphML export.
  Edge(Node* const out_node, Node* const in_node);

 private:
  // These pointers are not owning: the PageGraph instance owns them.
  Node* const out_node_;
  Node* const in_node_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::Edge> {
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return graph_item.IsEdge();
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_H_
