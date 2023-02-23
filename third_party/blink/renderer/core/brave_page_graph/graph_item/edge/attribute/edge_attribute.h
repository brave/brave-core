/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_ATTRIBUTE_EDGE_ATTRIBUTE_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_ATTRIBUTE_EDGE_ATTRIBUTE_H_

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/graph_edge.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace brave_page_graph {

class NodeActor;
class NodeHTMLElement;

class EdgeAttribute : public GraphEdge {
 public:
  EdgeAttribute(GraphItemContext* context,
                NodeActor* out_node,
                NodeHTMLElement* in_node,
                const String& name,
                const bool is_style = false);
  ~EdgeAttribute() override;

  const String& GetName() const { return name_; }
  bool IsStyle() const { return is_style_; }

  ItemDesc GetItemDesc() const override;

  void AddGraphMLAttributes(xmlDocPtr doc,
                            xmlNodePtr parent_node) const override;

  bool IsEdgeAttribute() const override;

  virtual bool IsEdgeAttributeDelete() const;
  virtual bool IsEdgeAttributeSet() const;

 private:
  const String name_;
  const bool is_style_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeAttribute> {
  static bool AllowFrom(const brave_page_graph::GraphEdge& edge) {
    return edge.IsEdgeAttribute();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeAttribute>(
        DynamicTo<brave_page_graph::GraphEdge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_ATTRIBUTE_EDGE_ATTRIBUTE_H_
