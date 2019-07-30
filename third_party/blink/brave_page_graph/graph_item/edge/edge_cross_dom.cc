/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "third_party/blink/renderer/core/dom/dom_node_ids.h"

#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_remote_frame.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_dom_root.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_frame_owner.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_cross_dom.h"

using ::std::string;
using ::std::to_string;

using ::blink::DOMNodeId;

namespace brave_page_graph {

EdgeCrossDOM::EdgeCrossDOM(PageGraph* const graph, NodeDOMRoot* const out_node,
    NodeDOMRoot* const in_node) :
      Edge(graph, out_node, in_node) {}

EdgeCrossDOM::EdgeCrossDOM(PageGraph* const graph,
    NodeFrameOwner* const out_node, NodeDOMRoot* const in_node) :
      Edge(graph, out_node, in_node) {}

EdgeCrossDOM::EdgeCrossDOM(PageGraph* const graph,
    NodeFrameOwner* const out_node, NodeRemoteFrame* const in_node) :
      Edge(graph, out_node, in_node) {}

EdgeCrossDOM::~EdgeCrossDOM() {}

ItemName EdgeCrossDOM::GetItemName() const {
  return "cross DOM";
}

bool EdgeCrossDOM::IsEdgeCrossDOM() const {
  return true;
}

}  // namespace brave_page_graph
