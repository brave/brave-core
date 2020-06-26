/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/webapi/edge_webapi_call.h"

#include <sstream>
#include <string>
#include <vector>

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_webapi.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_script.h"

using ::std::string;
using ::std::stringstream;
using ::std::vector;

namespace brave_page_graph {

EdgeWebAPICall::EdgeWebAPICall(PageGraph* const graph,
    NodeScript* const out_node, NodeWebAPI* const in_node,
    const MethodName& method, const vector<const string>& arguments) :
      EdgeWebAPI(graph, out_node, in_node, method),
      arguments_(arguments) {}

EdgeWebAPICall::~EdgeWebAPICall() {}

ItemName EdgeWebAPICall::GetItemName() const {
  return "web API call";
}

string EdgeWebAPICall::GetArgumentsString() const {
  stringstream builder;

  const size_t num_args = arguments_.size();
  const size_t last_index = num_args - 1;
  for (size_t i = 0; i < num_args; i += 1) {
    if (i == last_index) {
      builder << arguments_.at(i);
    } else {
      builder << arguments_.at(i) << ", ";
    }
  }

  return builder.str();
}

ItemDesc EdgeWebAPICall::GetItemDesc() const {
  return EdgeWebAPI::GetItemDesc()
      + " [arguments: " + GetArgumentsString() + "]";
}

GraphMLXMLList EdgeWebAPICall::GetGraphMLAttributes() const {
  GraphMLXMLList attrs = EdgeWebAPI::GetGraphMLAttributes();
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefCallArgs)
      ->ToValue(GetArgumentsString()));
  return attrs;
}

bool EdgeWebAPICall::IsEdgeWebAPICall() const {
  return true;
}

}  // namespace brave_page_graph
