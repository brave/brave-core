/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/edge_cross_dom.h"

#include <string>

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_actor.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_dom_root.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_frame_owner.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/node_remote_frame.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"

using ::blink::DOMNodeId;

namespace brave_page_graph {

EdgeCrossDOM::EdgeCrossDOM(GraphItemContext* context,
                           NodeFrameOwner* out_node,
                           NodeActor* in_node)
    : GraphEdge(context, out_node, in_node) {}

EdgeCrossDOM::EdgeCrossDOM(GraphItemContext* context,
                           NodeDOMRoot* out_node,
                           NodeDOMRoot* in_node)
    : GraphEdge(context, out_node, in_node) {}

EdgeCrossDOM::EdgeCrossDOM(GraphItemContext* context,
                           NodeFrameOwner* out_node,
                           NodeRemoteFrame* in_node)
    : GraphEdge(context, out_node, in_node) {}

EdgeCrossDOM::EdgeCrossDOM(GraphItemContext* context,
                           NodeActor* out_node,
                           NodeActor* in_node)
    : GraphEdge(context, out_node, in_node) {}

EdgeCrossDOM::~EdgeCrossDOM() = default;

ItemName EdgeCrossDOM::GetItemName() const {
  return "cross DOM";
}

bool EdgeCrossDOM::IsEdgeCrossDOM() const {
  return true;
}

}  // namespace brave_page_graph
