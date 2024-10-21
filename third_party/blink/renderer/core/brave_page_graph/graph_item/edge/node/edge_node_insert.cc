/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/node/edge_node_insert.h"

#include <sstream>
#include <string>

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_actor.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html_element.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html_text.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder_stream.h"

using ::blink::DOMNodeId;

namespace brave_page_graph {

EdgeNodeInsert::EdgeNodeInsert(GraphItemContext* context,
                               NodeActor* out_node,
                               NodeHTML* in_node,
                               const FrameId& frame_id,
                               NodeHTMLElement* parent_node,
                               NodeHTML* prior_sibling_node)
    : EdgeNode(context, out_node, in_node, frame_id),
      parent_node_(parent_node),
      prior_sibling_node_(prior_sibling_node) {}

EdgeNodeInsert::~EdgeNodeInsert() = default;

NodeHTMLElement* EdgeNodeInsert::GetParentNode() const {
  return parent_node_;
}

NodeHTML* EdgeNodeInsert::GetPriorSiblingNode() const {
  return prior_sibling_node_;
}

ItemName EdgeNodeInsert::GetItemName() const {
  return "insert node";
}

ItemDesc EdgeNodeInsert::GetItemDesc() const {
  const GraphNode* parent_node = GetParentNode();
  CHECK(parent_node);

  const GraphNode* prior_sibling_node = GetPriorSiblingNode();

  StringBuilder ts;
  ts << EdgeNode::GetItemDesc();
  ts << " [parent: " << parent_node->GetItemDesc() << "]";

  if (prior_sibling_node) {
    ts << " [prior sibling: " << prior_sibling_node->GetItemDesc() << "]";
  }

  return ts.ReleaseString();
}

void EdgeNodeInsert::AddGraphMLAttributes(xmlDocPtr doc,
                                          xmlNodePtr parent_node) const {
  EdgeNode::AddGraphMLAttributes(doc, parent_node);
  if (parent_node_) {
    GraphMLAttrDefForType(kGraphMLAttrDefParentNodeId)
        ->AddValueNode(doc, parent_node, parent_node_->GetDOMNodeId());
  }
  if (prior_sibling_node_) {
    GraphMLAttrDefForType(kGraphMLAttrDefBeforeNodeId)
        ->AddValueNode(doc, parent_node, prior_sibling_node_->GetDOMNodeId());
  }
}

bool EdgeNodeInsert::IsEdgeNodeInsert() const {
  return true;
}

}  // namespace brave_page_graph
