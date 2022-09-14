/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/edge_text_change.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_script.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html_text.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"

namespace brave_page_graph {

EdgeTextChange::EdgeTextChange(GraphItemContext* context,
                               NodeScript* out_node,
                               NodeHTMLText* in_node,
                               const std::string& text)
    : GraphEdge(context, out_node, in_node), text_(text) {}

ItemName EdgeTextChange::GetItemName() const {
  return "text change";
}

ItemName EdgeTextChange::GetItemDesc() const {
  return GraphEdge::GetItemDesc() + " [" + text_ + "]";
}

void EdgeTextChange::AddGraphMLAttributes(xmlDocPtr doc,
                                          xmlNodePtr parent_node) const {
  GraphEdge::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefValue)
      ->AddValueNode(doc, parent_node, text_);
}

bool EdgeTextChange::IsEdgeTextChange() const {
  return true;
}

}  // namespace brave_page_graph
