/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_frame_owner.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"

using ::blink::DOMNodeId;

namespace brave_page_graph {

NodeFrameOwner::NodeFrameOwner(GraphItemContext* context,
                               const DOMNodeId dom_node_id,
                               const String& tag_name)
    : NodeHTMLElement(context, dom_node_id, tag_name) {}

ItemName NodeFrameOwner::GetItemName() const {
  return "frame owner";
}

bool NodeFrameOwner::IsNodeFrameOwner() const {
  return true;
}

}  // namespace brave_page_graph
