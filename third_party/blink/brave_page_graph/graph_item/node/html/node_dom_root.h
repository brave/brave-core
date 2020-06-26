/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_HTML_NODE_DOM_ROOT_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_HTML_NODE_DOM_ROOT_H_

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_html_element.h"

namespace brave_page_graph {

class NodeDOMRoot final : public NodeHTMLElement {
friend class PageGraph;
 public:
  NodeDOMRoot() = delete;

  const std::string& GetURL() const { return url_; }

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  void AddGraphMLAttributes(xmlDocPtr doc, xmlNodePtr parent_node)
      const override;

  bool IsNodeDOMRoot() const override;

 protected:
  NodeDOMRoot(PageGraph* const graph, const blink::DOMNodeId node_id,
    const std::string& tag_name, const std::string& url);

 private:
  const std::string url_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeDOMRoot> {
  static bool AllowFrom(const brave_page_graph::NodeHTMLElement& element_node) {
    return element_node.IsNodeDOMRoot();
  }
  static bool AllowFrom(const brave_page_graph::NodeHTML& html_node) {
    return IsA<brave_page_graph::NodeDOMRoot>(
        DynamicTo<brave_page_graph::NodeHTMLElement>(html_node));
  }
  static bool AllowFrom(const brave_page_graph::Node& node) {
    return IsA<brave_page_graph::NodeDOMRoot>(
        DynamicTo<brave_page_graph::NodeHTML>(node));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeDOMRoot>(
        DynamicTo<brave_page_graph::Node>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_HTML_NODE_DOM_ROOT_H_
