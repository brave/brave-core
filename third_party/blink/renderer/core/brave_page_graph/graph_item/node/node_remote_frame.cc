/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/node_remote_frame.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/platform/wtf/text/text_stream.h"

namespace brave_page_graph {

NodeRemoteFrame::NodeRemoteFrame(GraphItemContext* context,
                                 const String& frame_id)
    : GraphNode(context), frame_id_(frame_id) {}

NodeRemoteFrame::~NodeRemoteFrame() = default;

ItemName NodeRemoteFrame::GetItemName() const {
  return "remote frame";
}

ItemDesc NodeRemoteFrame::GetItemDesc() const {
  WTF::TextStream ts;
  ts << GraphNode::GetItemDesc() << " [" << frame_id_ << "]";
  return ts.Release();
}

void NodeRemoteFrame::AddGraphMLAttributes(xmlDocPtr doc,
                                           xmlNodePtr parent_node) const {
  GraphNode::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefNodeFrameId)
      ->AddValueNode(doc, parent_node, frame_id_);
}

bool NodeRemoteFrame::IsNodeRemoteFrame() const {
  return true;
}

}  // namespace brave_page_graph
