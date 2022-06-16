/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/node_remote_frame.h"

#include <sstream>

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"

namespace brave_page_graph {

NodeRemoteFrame::NodeRemoteFrame(GraphItemContext* context,
                                 const std::string& frame_id)
    : GraphNode(context), frame_id_(frame_id) {}

NodeRemoteFrame::~NodeRemoteFrame() = default;

ItemName NodeRemoteFrame::GetItemName() const {
  return "remote frame";
}

ItemDesc NodeRemoteFrame::GetItemDesc() const {
  std::stringstream builder;
  builder << GraphNode::GetItemDesc() << " [" << frame_id_ << "]";
  return builder.str();
}

void NodeRemoteFrame::AddGraphMLAttributes(xmlDocPtr doc,
                                           xmlNodePtr parent_node) const {
  GraphNode::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefFrameId)
      ->AddValueNode(doc, parent_node, frame_id_);
}

bool NodeRemoteFrame::IsNodeRemoteFrame() const {
  return true;
}

}  // namespace brave_page_graph
