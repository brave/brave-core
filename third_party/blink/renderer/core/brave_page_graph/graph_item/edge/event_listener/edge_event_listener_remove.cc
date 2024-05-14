/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/event_listener/edge_event_listener_remove.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"

namespace brave_page_graph {

EdgeEventListenerRemove::EdgeEventListenerRemove(
    GraphItemContext* context,
    NodeActor* out_node,
    NodeHTMLElement* in_node,
    const FrameId& frame_id,
    const String& event_type,
    const EventListenerId listener_id,
    NodeActor* listener_script)
    : EdgeEventListenerAction(context,
                              out_node,
                              in_node,
                              frame_id,
                              event_type,
                              listener_id,
                              listener_script) {}

EdgeEventListenerRemove::~EdgeEventListenerRemove() = default;

ItemName EdgeEventListenerRemove::GetItemName() const {
  return "remove event listener";
}

bool EdgeEventListenerRemove::IsEdgeEventListenerRemove() const {
  return true;
}

}  // namespace brave_page_graph
