/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_event_listener_remove.h"
#include <string>

using ::std::string;
using ::std::to_string;

namespace brave_page_graph {

EdgeEventListenerRemove::EdgeEventListenerRemove(PageGraph* const graph,
    NodeActor* const out_node, NodeHTMLElement* const in_node,
    const string& event_type, const EventListenerId listener_id,
    const ScriptId listener_script_id) :
      EdgeEventListenerAction(graph, out_node, in_node, event_type,
                              listener_id, listener_script_id) {}

EdgeEventListenerRemove::~EdgeEventListenerRemove() {}

ItemName EdgeEventListenerRemove::GetItemName() const {
  return "remove event listener #" + to_string(id_);
}

const char* EdgeEventListenerRemove::GetEdgeType() const {
  return "remove event listener";
}

}  // namespace brave_page_graph
