/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EXECUTE_EDGE_EXECUTE_ATTR_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EXECUTE_EDGE_EXECUTE_ATTR_H_

#include <string>
#include <libxml/tree.h>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/execute/edge_execute.h"

namespace brave_page_graph {

class NodeHTMLElement;
class NodeScript;
class PageGraph;

class EdgeExecuteAttr : public EdgeExecute {
friend class PageGraph;
 public:
  EdgeExecuteAttr() = delete;
  ~EdgeExecuteAttr() override;

  const std::string& GetAttributeName() { return attribute_name_; }

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  void AddGraphMLAttributes(xmlDocPtr doc, xmlNodePtr parent_node)
      const override;

  bool IsEdgeExecuteAttr() const override;

 protected:
  EdgeExecuteAttr(PageGraph* const graph, NodeHTMLElement* const out_node,
    NodeScript* const in_node, const std::string& attribute_name);

 private:
  const std::string attribute_name_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeExecuteAttr> {
  static bool AllowFrom(const brave_page_graph::EdgeExecute& edge) {
    return edge.IsEdgeExecuteAttr();
  }
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return IsA<brave_page_graph::EdgeExecuteAttr>(
        DynamicTo<brave_page_graph::EdgeExecute>(edge));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeExecuteAttr>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EXECUTE_EDGE_EXECUTE_ATTR_H_
