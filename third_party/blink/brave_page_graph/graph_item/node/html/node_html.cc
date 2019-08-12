/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_html.h"

#include <libxml/tree.h>

#include "base/logging.h"

#include "third_party/blink/renderer/core/dom/dom_node_ids.h"

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/node/edge_node_delete.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

using ::std::stringstream;
using ::blink::DynamicTo;

namespace brave_page_graph {

NodeHTML::NodeHTML(PageGraph* const graph, const blink::DOMNodeId node_id) :
      Node(graph),
      node_id_(node_id),
      parent_node_(nullptr),
      is_deleted_(false) {}

NodeHTML::~NodeHTML() {}

void NodeHTML::MarkDeleted() {
  LOG_ASSERT(is_deleted_ == false);
  is_deleted_ = true;
}

ItemDesc NodeHTML::GetItemDesc() const {
  stringstream builder;
  builder << Node::GetItemDesc();
  if (is_deleted_) {
    builder << " [deleted]";
  }
  return builder.str();
}

void NodeHTML::AddGraphMLAttributes(xmlDocPtr doc,
    xmlNodePtr parent_node) const {
  Node::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefNodeId)
      ->AddValueNode(doc, parent_node, node_id_);
  GraphMLAttrDefForType(kGraphMLAttrDefIsDeleted)
      ->AddValueNode(doc, parent_node, is_deleted_);
}

void NodeHTML::AddInEdge(const Edge* const in_edge) {
  Node::AddInEdge(in_edge);
  if (const EdgeNodeDelete* const delete_node_in_edge =
          DynamicTo<EdgeNodeDelete>(in_edge)) {
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
