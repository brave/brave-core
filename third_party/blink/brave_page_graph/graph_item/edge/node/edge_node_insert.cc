/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/node/edge_node_insert.h"

#include <sstream>
#include <string>

#include "third_party/blink/renderer/core/dom/dom_node_ids.h"

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_actor.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_html.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_html_element.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_html_text.h"

using ::std::string;
using ::std::stringstream;
using ::std::to_string;

using ::blink::DOMNodeId;

namespace brave_page_graph {

EdgeNodeInsert::EdgeNodeInsert(PageGraph* const graph,
    NodeActor* const out_node, NodeHTML* const in_node,
    const DOMNodeId parent_node_id, const DOMNodeId prior_sibling_node_id) :
      EdgeNode(graph, out_node, in_node),
      parent_node_id_(parent_node_id),
      prior_sibling_node_id_(prior_sibling_node_id) {}

EdgeNodeInsert::~EdgeNodeInsert() {}

NodeHTMLElement* EdgeNodeInsert::GetParentNode() const {
  return parent_node_id_
      ? GetGraph()->GetHTMLElementNode(parent_node_id_)
      : nullptr;
}

NodeHTML* EdgeNodeInsert::GetPriorSiblingNode() const {
  return prior_sibling_node_id_
      ? GetGraph()->GetHTMLNode(prior_sibling_node_id_)
      : nullptr;
}

ItemName EdgeNodeInsert::GetItemName() const {
  return "insert node";
}

ItemDesc EdgeNodeInsert::GetItemDesc() const {
  const Node* const parent_node = GetParentNode();
  LOG_ASSERT(parent_node);

  const Node* const prior_sibling_node = GetPriorSiblingNode();

  stringstream builder;
  builder << EdgeNode::GetItemDesc();
  builder << " [parent: " << parent_node->GetItemDesc() << "]";

  if (prior_sibling_node) {
    builder << " [prior sibling: " << prior_sibling_node->GetItemDesc() << "]";
  }

  return builder.str();
}

GraphMLXMLList EdgeNodeInsert::GetGraphMLAttributes() const {
  GraphMLXMLList attrs = EdgeNode::GetGraphMLAttributes();
  if (parent_node_id_) {
    attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefParentNodeId)
        ->ToValue(parent_node_id_));
  }
  if (prior_sibling_node_id_) {
    attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefBeforeNodeId)
        ->ToValue(prior_sibling_node_id_));
  }
  return attrs;
}

bool EdgeNodeInsert::IsEdgeNodeInsert() const {
  return true;
}

}  // namespace brave_page_graph
