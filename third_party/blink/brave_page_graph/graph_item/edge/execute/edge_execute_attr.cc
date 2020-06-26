/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/execute/edge_execute_attr.h"

#include <string>
#include <libxml/tree.h>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_script.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_html_element.h"

using ::std::string;

namespace brave_page_graph {

EdgeExecuteAttr::EdgeExecuteAttr(PageGraph* const graph,
    NodeHTMLElement* const out_node, NodeScript* const in_node,
    const std::string& attribute_name) :
      EdgeExecute(graph, out_node, in_node),
      attribute_name_(attribute_name) {}

EdgeExecuteAttr::~EdgeExecuteAttr() {}

ItemName EdgeExecuteAttr::GetItemName() const {
  return "execute from attribute";
}

ItemDesc EdgeExecuteAttr::GetItemDesc() const {
  return EdgeExecute::GetItemDesc() + " [" + attribute_name_ + "]";
}

void EdgeExecuteAttr::AddGraphMLAttributes(xmlDocPtr doc,
    xmlNodePtr parent_node) const {
  EdgeExecute::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefAttrName)
      ->AddValueNode(doc, parent_node, attribute_name_);
}

bool EdgeExecuteAttr::IsEdgeExecuteAttr() const {
  return true;
}

}  // namespace brave_page_graph
