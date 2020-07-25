/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/js/node_js_builtin.h"

#include <libxml/tree.h>

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/js/node_js.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::string;

namespace brave_page_graph {

NodeJSBuiltIn::NodeJSBuiltIn(PageGraph* const graph, const JSBuiltIn built_in) :
      NodeJS(graph),
      built_in_(built_in) {}

NodeJSBuiltIn::~NodeJSBuiltIn() {}

JSBuiltIn NodeJSBuiltIn::GetBuiltIn() const {
  return built_in_;
}

const MethodName& NodeJSBuiltIn::GetMethodName() const {
  return JSBuiltInToSting(built_in_);
}

ItemName NodeJSBuiltIn::GetItemName() const {
  return "JS builtin";
}

ItemDesc NodeJSBuiltIn::GetItemDesc() const {
  return Node::GetItemDesc() + " [" + JSBuiltInToSting(built_in_) + "]";
}

void NodeJSBuiltIn::AddGraphMLAttributes(xmlDocPtr doc,
    xmlNodePtr parent_node) const {
  NodeJS::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefMethodName)
      ->AddValueNode(doc, parent_node, JSBuiltInToSting(built_in_));
}

bool NodeJSBuiltIn::IsNodeJSBuiltIn() const {
  return true;
}

}  // namespace brave_page_graph
