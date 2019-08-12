/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_HTML_NODE_HTML_ELEMENT_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_HTML_NODE_HTML_ELEMENT_H_

#include <map>
#include <string>
#include <libxml/tree.h>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_html.h"

namespace brave_page_graph {

class NodeHTMLText;
class PageGraph;

class NodeHTMLElement : public NodeHTML {
friend class NodeHTMLText;
friend class PageGraph;
 public:
  NodeHTMLElement() = delete;
  ~NodeHTMLElement() override;

  const std::string& TagName() const { return tag_name_; }

  const HTMLNodeList& GetChildNodes() const { return child_nodes_; }

  const AttributeMap& GetAttributes() const { return attributes_; }
  const AttributeMap& GetInlineStyles() const { return inline_styles_; }
  const EventListenerMap& GetEventListeners() const { return event_listeners_; }

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  void AddGraphMLTag(xmlDocPtr doc, xmlNodePtr parent_node) const override;
  void AddGraphMLAttributes(xmlDocPtr doc, xmlNodePtr parent_node)
      const override;

  bool IsNodeHTMLElement() const override;

  virtual bool IsNodeDOMRoot() const;
  virtual bool IsNodeFrameOwner() const;

 protected:
  NodeHTMLElement(PageGraph* const graph, const blink::DOMNodeId node_id,
    const std::string& tag_name);

  void PlaceChildNodeAfterSiblingNode(NodeHTML* const child,
    NodeHTML* const sibling);
  void RemoveChildNode(NodeHTML* const child_node);

  void MarkDeleted() override;

  void AddInEdge(const Edge* const in_edge) override;

 private:
  const std::string tag_name_;

  HTMLNodeList child_nodes_;

  AttributeMap attributes_;
  AttributeMap inline_styles_;
  EventListenerMap event_listeners_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeHTMLElement> {
  static bool AllowFrom(const brave_page_graph::NodeHTML& html_node) {
    return html_node.IsNodeHTMLElement();
  }
  static bool AllowFrom(const brave_page_graph::Node& node) {
    return IsA<brave_page_graph::NodeHTMLElement>(
        DynamicTo<brave_page_graph::NodeHTML>(node));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeHTMLElement>(
        DynamicTo<brave_page_graph::Node>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_HTML_NODE_HTML_ELEMENT_H_
