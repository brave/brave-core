/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/event_listener/edge_event_listener_add.h"

#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_actor.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_html_element.h"

using ::std::string;

namespace brave_page_graph {

EdgeEventListenerAdd::EdgeEventListenerAdd(PageGraph* const graph,
    NodeActor* const out_node, NodeHTMLElement* const in_node,
    const string& event_type, const EventListenerId listener_id,
    const ScriptId listener_script_id) :
      EdgeEventListenerAction(graph, out_node, in_node, event_type,
                              listener_id, listener_script_id) {}

EdgeEventListenerAdd::~EdgeEventListenerAdd() {}

ItemName EdgeEventListenerAdd::GetItemName() const {
  return "add event listener";
}

bool EdgeEventListenerAdd::IsEdgeEventListenerAdd() const {
  return true;
}

}  // namespace brave_page_graph
