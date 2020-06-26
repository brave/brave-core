/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_frame_owner.h"

#include <string>

#include "third_party/blink/renderer/core/dom/dom_node_ids.h"

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::string;
using ::std::to_string;

using ::blink::DOMNodeId;

namespace brave_page_graph {

NodeFrameOwner::NodeFrameOwner(PageGraph* const graph, const DOMNodeId node_id,
    const string& tag_name)
      : NodeHTMLElement(graph, node_id, tag_name) {}

ItemName NodeFrameOwner::GetItemName() const {
  return "frame owner";
}

bool NodeFrameOwner::IsNodeFrameOwner() const {
  return true;
}

}  // namespace brave_page_graph
