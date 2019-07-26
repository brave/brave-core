/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EVENT_LISTENER_EDGE_EVENT_LISTENER_ADD_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EVENT_LISTENER_EDGE_EVENT_LISTENER_ADD_H_

#include <string>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/event_listener/edge_event_listener_action.h"

namespace brave_page_graph {

class NodeActor;
class NodeHTMLElement;
class PageGraph;

class EdgeEventListenerAdd final : public EdgeEventListenerAction {
friend class PageGraph;
 public:
  EdgeEventListenerAdd() = delete;
  ~EdgeEventListenerAdd() override;

  ItemName GetItemName() const override;

  bool IsEdgeEventListenerAdd() const override;

 protected:
  EdgeEventListenerAdd(PageGraph* const graph, NodeActor* const out_node,
    NodeHTMLElement* const in_node, const std::string& event_type,
    const EventListenerId listener_id, const ScriptId listener_script_id);
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeEventListenerAdd> {
  static bool AllowFrom(
      const brave_page_graph::EdgeEventListenerAction&
          event_listener_action_edge) {
    return event_listener_action_edge.IsEdgeEventListenerAdd();
  }
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return IsA<brave_page_graph::EdgeEventListenerAdd>(
        DynamicTo<brave_page_graph::EdgeEventListenerAction>(edge));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeEventListenerAdd>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EVENT_LISTENER_EDGE_EVENT_LISTENER_ADD_H_
