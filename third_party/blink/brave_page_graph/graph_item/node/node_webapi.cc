/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_webapi.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::to_string;

namespace brave_page_graph {

NodeWebAPI::NodeWebAPI(PageGraph* const graph, const MethodName method) :
      Node(graph),
      method_name_(method) {}

NodeWebAPI::~NodeWebAPI() {}

ItemName NodeWebAPI::GetItemName() const {
  return "web API";
}

ItemDesc NodeWebAPI::GetItemDesc() const {
  return Node::GetItemDesc() + " [" + method_name_ + "]";
}

GraphMLXMLList NodeWebAPI::GetGraphMLAttributes() const {
  GraphMLXMLList attrs = Node::GetGraphMLAttributes();
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefMethodName)
      ->ToValue(method_name_));
  return attrs;
}

bool NodeWebAPI::IsNodeWebAPI() const {
  return true;
}

}  // namespace brave_page_graph
