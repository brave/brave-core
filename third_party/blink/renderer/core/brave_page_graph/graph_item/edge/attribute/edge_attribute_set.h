/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_ATTRIBUTE_EDGE_ATTRIBUTE_SET_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_ATTRIBUTE_EDGE_ATTRIBUTE_SET_H_

#include <string>

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/attribute/edge_attribute.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace brave_page_graph {

class NodeActor;
class NodeHTMLElement;

class EdgeAttributeSet final : public EdgeAttribute {
 public:
  EdgeAttributeSet(GraphItemContext* context,
                   NodeActor* out_node,
                   NodeHTMLElement* in_node,
                   const std::string& name,
                   const std::string& value,
                   const bool is_style = false);
  ~EdgeAttributeSet() override;

  const std::string& GetValue() const { return value_; }

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  void AddGraphMLAttributes(xmlDocPtr doc,
                            xmlNodePtr parent_node) const override;

  bool IsEdgeAttributeSet() const override;

 private:
  const std::string value_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeAttributeSet> {
  static bool AllowFrom(const brave_page_graph::EdgeAttribute& attribute_edge) {
    return attribute_edge.IsEdgeAttributeSet();
  }
  static bool AllowFrom(const brave_page_graph::GraphEdge& edge) {
    return IsA<brave_page_graph::EdgeAttributeSet>(
        DynamicTo<brave_page_graph::EdgeAttribute>(edge));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeAttributeSet>(
        DynamicTo<brave_page_graph::GraphEdge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_ATTRIBUTE_EDGE_ATTRIBUTE_SET_H_
