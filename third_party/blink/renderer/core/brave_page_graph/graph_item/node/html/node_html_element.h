/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_HTML_NODE_HTML_ELEMENT_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_HTML_NODE_HTML_ELEMENT_H_

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/graph_node.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"
#include "third_party/blink/renderer/platform/wtf/text/string_hash.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace brave_page_graph {

class EdgeEventListenerAdd;

class NodeHTMLElement : public NodeHTML {
 public:
  using Attributes = HashMap<String, String>;

  NodeHTMLElement(GraphItemContext* context,
                  const blink::DOMNodeId dom_node_id,
                  const String& tag_name);
  ~NodeHTMLElement() override;

  const String& TagName() const { return tag_name_; }

  const HTMLNodeList& GetChildNodes() const { return child_nodes_; }

  const Attributes& GetAttributes() const { return attributes_; }
  const Attributes& GetInlineStyles() const { return inline_styles_; }

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  void AddGraphMLTag(xmlDocPtr doc, xmlNodePtr parent_node) const override;
  void AddGraphMLAttributes(xmlDocPtr doc,
                            xmlNodePtr parent_node) const override;

  bool IsNodeHTMLElement() const override;

  virtual bool IsNodeDOMRoot() const;
  virtual bool IsNodeFrameOwner() const;

  void PlaceChildNodeAfterSiblingNode(NodeHTML* child, NodeHTML* sibling);
  void RemoveChildNode(NodeHTML* child_node);
  void MarkDeleted() override;

 protected:
  void AddInEdge(const GraphEdge* in_edge) override;

 private:
  const String tag_name_;

  HTMLNodeList child_nodes_;

  Attributes attributes_;
  Attributes inline_styles_;
  ZeroBasedHashMap<EventListenerId, const EdgeEventListenerAdd*>
      event_listeners_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeHTMLElement> {
  static bool AllowFrom(const brave_page_graph::NodeHTML& html_node) {
    return html_node.IsNodeHTMLElement();
  }
  static bool AllowFrom(const brave_page_graph::GraphNode& node) {
    return IsA<brave_page_graph::NodeHTMLElement>(
        DynamicTo<brave_page_graph::NodeHTML>(node));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeHTMLElement>(
        DynamicTo<brave_page_graph::GraphNode>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_HTML_NODE_HTML_ELEMENT_H_
