/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_ATTRIBUTE_EDGE_ATTRIBUTE_DELETE_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_ATTRIBUTE_EDGE_ATTRIBUTE_DELETE_H_

#include <string>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/attribute/edge_attribute.h"

namespace brave_page_graph {

class Node;
class NodeActor;
class NodeHTMLElement;
class PageGraph;

class EdgeAttributeDelete final : public EdgeAttribute {
friend class PageGraph;
 public:
  EdgeAttributeDelete() = delete;
  ~EdgeAttributeDelete() override;

  ItemName GetItemName() const override;

  bool IsEdgeAttributeDelete() const override;

 protected:
  EdgeAttributeDelete(PageGraph* const graph, NodeActor* const out_node,
    NodeHTMLElement* const in_node, const std::string& name,
    const bool is_style = false);
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeAttributeDelete> {
  static bool AllowFrom(const brave_page_graph::EdgeAttribute& attribute_edge) {
    return attribute_edge.IsEdgeAttributeDelete();
  }
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return IsA<brave_page_graph::EdgeAttributeDelete>(
        DynamicTo<brave_page_graph::EdgeAttribute>(edge));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeAttributeDelete>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_ATTRIBUTE_EDGE_ATTRIBUTE_DELETE_H_
