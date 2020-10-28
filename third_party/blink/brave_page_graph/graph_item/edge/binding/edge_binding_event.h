/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_BINDING_EDGE_BINDING_EVENT_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_BINDING_EDGE_BINDING_EVENT_H_

#include <string>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"

namespace brave_page_graph {

class NodeBindingEvent;
class NodeScript;
class PageGraph;

class EdgeBindingEvent : public Edge {
friend class PageGraph;
 public:
  EdgeBindingEvent() = delete;
  ~EdgeBindingEvent() override;

  ScriptPosition GetScriptPosition() const { return script_position_; }

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  void AddGraphMLAttributes(xmlDocPtr doc, xmlNodePtr parent_node)
      const override;

  bool IsEdgeBindingEvent() const override;

 protected:
  EdgeBindingEvent(PageGraph* const graph, NodeScript* const out_node,
    NodeBindingEvent* const in_node, const ScriptPosition script_position);

 private:
  const ScriptPosition script_position_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeBindingEvent> {
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return edge.IsEdgeBindingEvent();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeBindingEvent>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_BINDING_EDGE_BINDING_EVENT_H_
