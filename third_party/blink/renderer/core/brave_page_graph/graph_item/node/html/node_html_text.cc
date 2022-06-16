/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html_text.h"

#include "base/strings/string_number_conversions.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/edge_text_change.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/node/edge_node_create.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/node/edge_node_delete.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/node/edge_node_insert.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/node/edge_node_remove.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html_element.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"

using ::blink::DOMNodeId;
using ::blink::DynamicTo;

namespace brave_page_graph {

NodeHTMLText::NodeHTMLText(GraphItemContext* context,
                           const DOMNodeId dom_node_id,
                           const std::string& text)
    : NodeHTML(context, dom_node_id), text_(text) {}

NodeHTMLText::~NodeHTMLText() = default;

ItemName NodeHTMLText::GetItemName() const {
  return "text node";
}

ItemDesc NodeHTMLText::GetItemDesc() const {
  return NodeHTML::GetItemDesc() +
         " [length: " + base::NumberToString(text_.size()) + "]";
}

void NodeHTMLText::AddGraphMLAttributes(xmlDocPtr doc,
                                        xmlNodePtr parent_node) const {
  NodeHTML::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefNodeText)
      ->AddValueNode(doc, parent_node, text_);
}

void NodeHTMLText::AddInEdge(const GraphEdge* in_edge) {
  NodeHTML::AddInEdge(in_edge);
  if (const EdgeNodeRemove* remove_node_in_edge =
          DynamicTo<EdgeNodeRemove>(in_edge)) {
    // HTML Text nodes can't be removed if they were never inserted into the
    // tree.
    if (GetParentNode()) {
      GetParentNode()->RemoveChildNode(this);
    }
    SetParentNode(nullptr);
  } else if (const EdgeNodeInsert* insert_node_in_edge =
                 DynamicTo<EdgeNodeInsert>(in_edge)) {
    SetParentNode(insert_node_in_edge->GetParentNode());
    GetParentNode()->PlaceChildNodeAfterSiblingNode(
        this, insert_node_in_edge->GetPriorSiblingNode());
  } else if (const EdgeTextChange* text_change_in_edge =
                 DynamicTo<EdgeTextChange>(in_edge)) {
    text_ = text_change_in_edge->GetText();
  }
}

bool NodeHTMLText::IsNodeHTMLText() const {
  return true;
}

}  // namespace brave_page_graph
