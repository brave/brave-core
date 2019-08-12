/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/js/node_js_webapi.h"

#include <libxml/tree.h>

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/js/node_js.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

NodeJSWebAPI::NodeJSWebAPI(PageGraph* const graph, const MethodName method) :
      NodeJS(graph),
      method_name_(method) {}

NodeJSWebAPI::~NodeJSWebAPI() {}

ItemName NodeJSWebAPI::GetItemName() const {
  return "web API";
}

ItemDesc NodeJSWebAPI::GetItemDesc() const {
  return Node::GetItemDesc() + " [" + method_name_ + "]";
}

void NodeJSWebAPI::AddGraphMLAttributes(xmlDocPtr doc,
    xmlNodePtr parent_node) const {
  NodeJS::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefMethodName)
      ->AddValueNode(doc, parent_node, method_name_);
}

const MethodName& NodeJSWebAPI::GetMethodName() const {
  return method_name_;
}

bool NodeJSWebAPI::IsNodeJSWebAPI() const {
  return true;
}

}  // namespace brave_page_graph
