/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html_text.h"
#include <string>
#include "base/logging.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html_element.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_remove.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_insert.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_delete.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::blink::DOMNodeId;
using ::std::string;
using ::std::to_string;

namespace brave_page_graph {

NodeHTMLText::NodeHTMLText(PageGraph* const graph,
    const DOMNodeId node_id,  const string& text) :
      NodeHTML(graph, node_id),
      text_(text) {}

NodeHTMLText::~NodeHTMLText() {}

ItemName NodeHTMLText::GetItemName() const {
  return "TextNode (length: "  + to_string(text_.size()) + ")";
}

const string& NodeHTMLText::Text() const {
  return text_;
}

GraphMLXMLList NodeHTMLText::GraphMLAttributes() const {
  GraphMLXMLList attrs = NodeHTML::GraphMLAttributes();
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefNodeType)
      ->ToValue("text node"));
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefNodeText)
      ->ToValue(Text()));
  return attrs;
}

ItemDesc NodeHTMLText::GetDescBody() const {
  return "(text)" + text_;
}

void NodeHTMLText::AddInEdge(const EdgeNodeRemove* const edge) {
  parent_node_->RemoveChildNode(this);
  parent_node_ = nullptr;
  Node::AddInEdge(edge);
}

void NodeHTMLText::AddInEdge(const EdgeNodeInsert* const edge) {
  parent_node_ = edge->GetParentNode();
  parent_node_->PlaceChildNodeAfterSiblingNode(this,
    edge->GetPriorSiblingNode());
  Node::AddInEdge(edge);
}

void NodeHTMLText::AddInEdge(const EdgeNodeDelete* const edge) {
  MarkNodeDeleted();
  Node::AddInEdge(edge);
}

}  // namespace brave_page_graph
