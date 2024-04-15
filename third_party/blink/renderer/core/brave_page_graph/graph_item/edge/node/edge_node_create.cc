/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/node/edge_node_create.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_actor.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"

namespace brave_page_graph {

EdgeNodeCreate::EdgeNodeCreate(GraphItemContext* context,
                               NodeActor* out_node,
                               NodeHTML* in_node,
                               const FrameId& frame_id)
    : EdgeNode(context, out_node, in_node, frame_id) {}

EdgeNodeCreate::~EdgeNodeCreate() = default;

ItemName EdgeNodeCreate::GetItemName() const {
  return "create node";
}

bool EdgeNodeCreate::IsEdgeNodeCreate() const {
  return true;
}

}  // namespace brave_page_graph
