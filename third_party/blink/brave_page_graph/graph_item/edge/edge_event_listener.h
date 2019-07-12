/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_EVENT_LISTENER_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_EVENT_LISTENER_H_

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

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
  ItemName GetItemName() const override;

 protected:
  EdgeEventListener(PageGraph* const graph, NodeHTMLElement* const out_node,
    NodeActor* const in_node, const std::string& event_type,
    const EventListenerId listener_id);

  // Only used for generating edges during GraphML export.
  EdgeEventListener(const NodeHTMLElement* const out_node,
    NodeActor* const in_node, const std::string& event_type,
    const EventListenerId listener_id);
  GraphMLXMLList GraphMLAttributes() const override;

 private:
  const std::string event_type_;
  const EventListenerId listener_id_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_EVENT_LISTENER_H_
