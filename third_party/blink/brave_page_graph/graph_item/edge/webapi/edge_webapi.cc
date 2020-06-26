/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/webapi/edge_webapi.h"

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_script.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_webapi.h"

using ::std::string;

namespace brave_page_graph {

EdgeWebAPI::EdgeWebAPI(PageGraph* const graph, Node* const out_node,
    Node* const in_node, const string& method) :
      Edge(graph, out_node, in_node),
      method_(method) {}

EdgeWebAPI::~EdgeWebAPI() {}

ItemName EdgeWebAPI::GetItemDesc() const {
  return Edge::GetItemDesc() + " [" + method_ + "]";
}

GraphMLXMLList EdgeWebAPI::GetGraphMLAttributes() const {
  GraphMLXMLList attrs = Edge::GetGraphMLAttributes();
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefKey)
      ->ToValue(method_));
  return attrs;
}

bool EdgeWebAPI::IsEdgeWebAPI() const {
  return true;
}

bool EdgeWebAPI::IsEdgeWebAPICall() const {
  return false;
}

bool EdgeWebAPI::IsEdgeWebAPIResult() const {
  return false;
}

}  // namespace brave_page_graph
