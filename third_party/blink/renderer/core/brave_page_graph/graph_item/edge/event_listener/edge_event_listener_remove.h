/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EVENT_LISTENER_EDGE_EVENT_LISTENER_REMOVE_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EVENT_LISTENER_EDGE_EVENT_LISTENER_REMOVE_H_

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/event_listener/edge_event_listener_action.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace brave_page_graph {

class NodeActor;
class NodeHTMLElement;

class EdgeEventListenerRemove final : public EdgeEventListenerAction {
 public:
  EdgeEventListenerRemove(GraphItemContext* context,
                          NodeActor* out_node,
                          NodeHTMLElement* in_node,
                          const FrameId& frame_id,
                          const String& event_type,
                          const EventListenerId listener_id,
                          NodeActor* listener_script);
  ~EdgeEventListenerRemove() override;

  ItemName GetItemName() const override;

  bool IsEdgeEventListenerRemove() const override;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeEventListenerRemove> {
  static bool AllowFrom(const brave_page_graph::EdgeEventListenerAction&
                            event_listener_action_edge) {
    return event_listener_action_edge.IsEdgeEventListenerRemove();
  }
  static bool AllowFrom(const brave_page_graph::GraphEdge& edge) {
    return IsA<brave_page_graph::EdgeEventListenerRemove>(
        DynamicTo<brave_page_graph::EdgeEventListenerAction>(edge));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeEventListenerRemove>(
        DynamicTo<brave_page_graph::GraphEdge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EVENT_LISTENER_EDGE_EVENT_LISTENER_REMOVE_H_
