/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/node/edge_node_delete.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder_stream.h"

using ::blink::DynamicTo;

namespace brave_page_graph {

NodeHTML::NodeHTML(GraphItemContext* context,
                   const blink::DOMNodeId dom_node_id)
    : GraphNode(context),
      dom_node_id_(dom_node_id),
      parent_node_(nullptr),
      is_deleted_(false) {}

NodeHTML::~NodeHTML() = default;

void NodeHTML::MarkDeleted() {
  CHECK(is_deleted_ == false);
  is_deleted_ = true;
}

ItemDesc NodeHTML::GetItemDesc() const {
  StringBuilder ts;
  ts << GraphNode::GetItemDesc();
  if (is_deleted_) {
    ts << " [deleted]";
  }
  return ts.ReleaseString();
}

void NodeHTML::AddGraphMLAttributes(xmlDocPtr doc,
                                    xmlNodePtr parent_node) const {
  GraphNode::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefNodeId)
      ->AddValueNode(doc, parent_node, dom_node_id_);
  GraphMLAttrDefForType(kGraphMLAttrDefIsDeleted)
      ->AddValueNode(doc, parent_node, is_deleted_);
}

void NodeHTML::AddInEdge(const GraphEdge* in_edge) {
  GraphNode::AddInEdge(in_edge);
  if (DynamicTo<EdgeNodeDelete>(in_edge)) {
    MarkDeleted();
  }
}

bool NodeHTML::IsNodeHTML() const {
  return true;
}

bool NodeHTML::IsNodeHTMLElement() const {
  return false;
}

bool NodeHTML::IsNodeHTMLText() const {
  return false;
}

}  // namespace brave_page_graph
