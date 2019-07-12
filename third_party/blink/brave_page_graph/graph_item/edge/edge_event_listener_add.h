/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_EVENT_LISTENER_ADD_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_EVENT_LISTENER_ADD_H_

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_event_listener_action.h"

namespace brave_page_graph {

class EdgeEventListenerAdd final : public EdgeEventListenerAction {
friend class PageGraph;
 public:
  EdgeEventListenerAdd() = delete;
  ~EdgeEventListenerAdd() override;

  ItemName GetItemName() const override;
  const char* GetEdgeType() const override;

 protected:
  EdgeEventListenerAdd(PageGraph* const graph, NodeActor* const out_node,
    NodeHTMLElement* const in_node, const std::string& event_type,
    const EventListenerId listener_id, const ScriptId listener_script_id);
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_EVENT_LISTENER_ADD_H_
