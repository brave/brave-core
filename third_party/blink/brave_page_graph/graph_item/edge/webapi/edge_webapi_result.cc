/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/webapi/edge_webapi_result.h"

#include <string>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_webapi.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_script.h"

namespace brave_page_graph {

EdgeWebAPIResult::EdgeWebAPIResult(PageGraph* const graph,
    NodeWebAPI* const out_node, NodeScript* const in_node,
    const MethodName& method, const std::string& result) :
      EdgeWebAPI(graph, out_node, in_node, method),
      result_(result) {}

EdgeWebAPIResult::~EdgeWebAPIResult() {}

ItemName EdgeWebAPIResult::GetItemName() const {
  return "web API result";
}

ItemDesc EdgeWebAPIResult::GetItemDesc() const {
  return EdgeWebAPI::GetItemDesc() + " [result: " + result_ + "]";
}

GraphMLXMLList EdgeWebAPIResult::GetGraphMLAttributes() const {
  GraphMLXMLList attrs = EdgeWebAPI::GetGraphMLAttributes();
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefValue)
      ->ToValue(result_));
  return attrs;
}

bool EdgeWebAPIResult::IsEdgeWebAPIResult() const {
  return true;
}

}  // namespace brave_page_graph
