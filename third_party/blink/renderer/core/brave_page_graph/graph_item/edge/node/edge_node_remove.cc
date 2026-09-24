/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/node/edge_node_remove.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_script.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html.h"

namespace brave_page_graph {

EdgeNodeRemove::EdgeNodeRemove(GraphItemContext* context,
                               NodeScript* out_node,
                               NodeHTML* in_node,
                               const FrameId& frame_id)
    : EdgeNode(context, out_node, in_node, frame_id) {}

EdgeNodeRemove::~EdgeNodeRemove() = default;

ItemName EdgeNodeRemove::GetItemName() const {
  return "remove node";
}

bool EdgeNodeRemove::IsEdgeNodeRemove() const {
  return true;
}

}  // namespace brave_page_graph
