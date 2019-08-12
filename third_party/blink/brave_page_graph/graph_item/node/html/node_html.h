/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_HTML_NODE_HTML_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_HTML_NODE_HTML_H_

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

namespace brave_page_graph {

class NodeHTMLElement;
class PageGraph;

class NodeHTML : public Node {
// Needed to propagate MarkDeleted to children.
friend class NodeHTMLElement;
friend class PageGraph;
 public:
  NodeHTML() = delete;
  ~NodeHTML() override;

  blink::DOMNodeId GetNodeId() const { return node_id_; }

  NodeHTMLElement* GetParentNode() const { return parent_node_; }

  bool IsDeleted() const { return is_deleted_; }

  ItemDesc GetItemDesc() const override;

  void AddGraphMLAttributes(xmlDocPtr doc, xmlNodePtr parent_node)
      const override;

  bool IsNodeHTML() const override;

  virtual bool IsNodeHTMLElement() const;
  virtual bool IsNodeHTMLText() const;

 protected:
  NodeHTML(PageGraph* const graph, const blink::DOMNodeId node_id);

  void SetParentNode(NodeHTMLElement* const parent_node) {
    parent_node_ = parent_node;
  }

  virtual void MarkDeleted();

  void AddInEdge(const Edge* const in_edge) override;

 private:
  const blink::DOMNodeId node_id_;
  NodeHTMLElement* parent_node_;
  bool is_deleted_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeHTML> {
  static bool AllowFrom(const brave_page_graph::Node& node) {
    return node.IsNodeHTML();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeHTML>(
        DynamicTo<brave_page_graph::Node>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_HTML_NODE_HTML_H_
