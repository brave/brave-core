/* Copyright (c) 2019 The Brave Software Team. DiEventListeneributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EVENT_LISTENER_EDGE_EVENT_LISTENER_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EVENT_LISTENER_EDGE_EVENT_LISTENER_H_

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"

namespace brave_page_graph {

class NodeActor;
class NodeHTMLElement;
class PageGraph;

class EdgeEventListener : public Edge {
// Needed for generating edges during GraphML export
friend class NodeHTMLElement;
friend class PageGraph;
 public:
  EdgeEventListener() = delete;
  ~EdgeEventListener() override;

  const std::string& GetEventType() const { return event_type_; }
  EventListenerId GetListenerId() const { return listener_id_; }

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  GraphMLXMLList GetGraphMLAttributes() const override;

  bool IsEdgeEventListener() const override;

 protected:
  EdgeEventListener(PageGraph* const graph, NodeHTMLElement* const out_node,
    NodeActor* const in_node, const std::string& event_type,
    const EventListenerId listener_id);
  // Only used for generating edges during GraphML export.
  EdgeEventListener(const NodeHTMLElement* const out_node,
    NodeActor* const in_node, const std::string& event_type,
    const EventListenerId listener_id);

 private:
  const std::string event_type_;
  const EventListenerId listener_id_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeEventListener> {
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return edge.IsEdgeEventListener();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeEventListener>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EVENT_LISTENER_EDGE_EVENT_LISTENER_H_
