/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_GRAPH_EDGE_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_GRAPH_EDGE_H_

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/graph_item.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace brave_page_graph {

class GraphNode;

class GraphEdge : public GraphItem {
 public:
  GraphEdge(GraphItemContext* context, GraphNode* out_node, GraphNode* in_node);

  ~GraphEdge() override;

  GraphNode* GetOutNode() const { return out_node_; }
  GraphNode* GetInNode() const { return in_node_; }

  GraphMLId GetGraphMLId() const override;
  void AddGraphMLTag(xmlDocPtr doc, xmlNodePtr parent_node) const override;
  void AddGraphMLAttributes(xmlDocPtr doc,
                            xmlNodePtr parent_node) const override;

  bool IsEdge() const override;

  virtual bool IsEdgeAttribute() const;
  virtual bool IsEdgeBinding() const;
  virtual bool IsEdgeBindingEvent() const;
  virtual bool IsEdgeCrossDOM() const;
  virtual bool IsEdgeEventListener() const;
  virtual bool IsEdgeEventListenerAction() const;
  virtual bool IsEdgeExecute() const;
  virtual bool IsEdgeFilter() const;
  virtual bool IsEdgeJS() const;
  virtual bool IsEdgeNode() const;
  virtual bool IsEdgeRequest() const;
  virtual bool IsEdgeResourceBlock() const;
  virtual bool IsEdgeShield() const;
  virtual bool IsEdgeStorage() const;
  virtual bool IsEdgeStorageBucket() const;
  virtual bool IsEdgeStructure() const;
  virtual bool IsEdgeTextChange() const;

 private:
  // These pointers are not owning: the GraphItemContext instance owns them.
  GraphNode* const out_node_;
  GraphNode* const in_node_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::GraphEdge> {
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return graph_item.IsEdge();
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_GRAPH_EDGE_H_
