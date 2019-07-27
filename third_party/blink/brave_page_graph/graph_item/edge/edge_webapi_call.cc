/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_webapi_call.h"
#include <sstream>
#include <string>
#include <vector>
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_webapi.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_script.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_webapi.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::string;
using ::std::stringstream;
using ::std::to_string;
using ::std::vector;

namespace brave_page_graph {

EdgeWebAPICall::EdgeWebAPICall(PageGraph* const graph,
    NodeScript* const out_node, NodeWebAPI* const in_node,
    const MethodName& method, const vector<const string>& arguments) :
      EdgeWebAPI(graph, out_node, in_node, method),
      arguments_(arguments) {}

EdgeWebAPICall::~EdgeWebAPICall() {}

ItemName EdgeWebAPICall::GetItemName() const {
  return "web API call #" + to_string(id_);
}

const vector<const string>& EdgeWebAPICall::GetArguments() const {
  return arguments_;
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

ItemDesc EdgeWebAPICall::GetDescBody() const {
  return GetItemName() + " (" + method_ + "; arguments: " + GetArgumentsString() + ")";
}

GraphMLXMLList EdgeWebAPICall::GraphMLAttributes() const {
  GraphMLXMLList attrs;
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefEdgeType)
      ->ToValue("webapi call"));
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefKey)
      ->ToValue(method_));
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefCallArgs)
      ->ToValue(GetArgumentsString()));
  return attrs;
}

}  // namespace brave_page_graph
