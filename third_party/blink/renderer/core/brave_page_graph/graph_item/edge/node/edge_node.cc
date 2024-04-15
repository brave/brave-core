/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/node/edge_node.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_actor.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"

namespace brave_page_graph {

EdgeNode::EdgeNode(GraphItemContext* context,
                   NodeActor* out_node,
                   NodeHTML* in_node,
                   const FrameId& frame_id)
    : GraphEdge(context, out_node, in_node), frame_id_(frame_id) {}

EdgeNode::~EdgeNode() = default;

bool EdgeNode::IsEdgeNode() const {
  return true;
}

bool EdgeNode::IsEdgeNodeCreate() const {
  return false;
}

bool EdgeNode::IsEdgeNodeDelete() const {
  return false;
}

bool EdgeNode::IsEdgeNodeInsert() const {
  return false;
}

bool EdgeNode::IsEdgeNodeRemove() const {
  return false;
}

void EdgeNode::AddGraphMLAttributes(xmlDocPtr doc,
                                    xmlNodePtr parent_node) const {
  GraphEdge::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefEdgeFrameId)
      ->AddValueNode(doc, parent_node, frame_id_);
}

}  // namespace brave_page_graph
