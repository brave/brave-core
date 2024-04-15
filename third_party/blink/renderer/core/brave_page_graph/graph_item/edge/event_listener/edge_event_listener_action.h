/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EVENT_LISTENER_EDGE_EVENT_LISTENER_ACTION_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EVENT_LISTENER_EDGE_EVENT_LISTENER_ACTION_H_

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/graph_edge.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace brave_page_graph {

class NodeActor;
class NodeHTMLElement;

class EdgeEventListenerAction : public GraphEdge {
 public:
  EdgeEventListenerAction(GraphItemContext* context,
                          NodeActor* out_node,
                          NodeHTMLElement* in_node,
                          const FrameId& frame_id,
                          const String& event_type,
                          const EventListenerId listener_id,
                          NodeActor* listener_script);

  ~EdgeEventListenerAction() override;

  const String& GetEventType() const { return event_type_; }
  EventListenerId GetListenerId() const { return listener_id_; }
  NodeActor* GetListenerNode() const { return listener_script_; }
  ScriptId GetListenerScriptId() const;

  ItemDesc GetItemDesc() const override;

  void AddGraphMLAttributes(xmlDocPtr doc,
                            xmlNodePtr parent_node) const override;

  bool IsEdgeEventListenerAction() const override;

  virtual bool IsEdgeEventListenerAdd() const;
  virtual bool IsEdgeEventListenerRemove() const;

 private:
  const FrameId frame_id_;
  const String event_type_;
  const EventListenerId listener_id_;
  NodeActor* listener_script_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeEventListenerAction> {
  static bool AllowFrom(const brave_page_graph::GraphEdge& edge) {
    return edge.IsEdgeEventListenerAction();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeEventListenerAction>(
        DynamicTo<brave_page_graph::GraphEdge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EVENT_LISTENER_EDGE_EVENT_LISTENER_ACTION_H_
