/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_H_

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/graph_item.h"

namespace brave_page_graph {

class Edge;
class PageGraph;

class Node : public GraphItem {
friend class PageGraph;
 public:
  Node() = delete;
  ~Node() override;

  const EdgeList& GetInEdges() const { return in_edges_; }
  const EdgeList& GetOutEdges() const { return out_edges_; }

  GraphMLId GetGraphMLId() const override;
  GraphMLXML GetGraphMLTag() const override;
  GraphMLXMLList GetGraphMLAttributes() const override;

  bool IsNode() const override;

  virtual bool IsNodeActor() const;
  virtual bool IsNodeFilter() const;
  virtual bool IsNodeHTML() const;
  virtual bool IsNodeExtensions() const;
  virtual bool IsNodeRemoteFrame() const;
  virtual bool IsNodeResource() const;
  virtual bool IsNodeShield() const;
  virtual bool IsNodeShields() const;
  virtual bool IsNodeStorage() const;
  virtual bool IsNodeStorageRoot() const;
  virtual bool IsNodeWebAPI() const;

 protected:
  Node(PageGraph* const graph);

  virtual void AddInEdge(const Edge* const in_edge);
  virtual void AddOutEdge(const Edge* const out_edge);

 private:
  // Reminder to self:
  //   out_edge -> node -> in_edge
  // These vectors do not own their references.  All nodes in the entire
  // graph are owned by the PageGraph instance.
  EdgeList in_edges_;
  EdgeList out_edges_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::Node> {
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return graph_item.IsNode();
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_H_
