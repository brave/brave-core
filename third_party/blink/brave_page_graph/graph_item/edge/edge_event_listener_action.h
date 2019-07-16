/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_EVENT_LISTENER_ACTION_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_EVENT_LISTENER_ACTION_H_

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"
#include "brave/third_party/blink/brave_page_graph/types.h"
#include <string>

namespace brave_page_graph {

class NodeActor;
class NodeHTMLElement;
class PageGraph;

class EdgeEventListenerAction : public Edge {
friend class PageGraph;
 public:
  EdgeEventListenerAction() = delete;

  const std::string& GetEventType() const { return event_type_; }
  EventListenerId GetListenerId() const { return listener_id_; }
  ScriptId GetListenerScriptId() const { return listener_script_id_; }

 protected:
  EdgeEventListenerAction(PageGraph* const graph, NodeActor* const out_node,
    NodeHTMLElement* const in_node, const std::string& event_type,
    const EventListenerId listener_id, const ScriptId listener_script_id);

  ItemDesc GetDescBody() const override;
  GraphMLXMLList GraphMLAttributes() const override;

  virtual const char* GetEdgeType() const = 0;

private:
  const std::string event_type_;
  const EventListenerId listener_id_;
  const ScriptId listener_script_id_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_NODE_H_
