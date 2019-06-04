/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_call.h"
#include <sstream>
#include <string>
#include <vector>
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"
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

EdgeCall::EdgeCall(PageGraph* const graph, NodeScript* const out_node,
    NodeWebAPI* const in_node, const string& method,
    const vector<const string>& arguments) :
      Edge(graph, out_node, in_node),
      method_(method),
      arguments_(arguments) {}

EdgeCall::~EdgeCall() {}

ItemName EdgeCall::GetItemName() const {
  return "EdgeCall#" + ::std::to_string(id_);
}

const vector<const string>& EdgeCall::GetArguments() const {
  return arguments_;
}

string EdgeCall::GetArgumentsString() const {
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

ItemDesc EdgeCall::GetDescBody() const {
  return GetItemName() + " [method: " + method_
                    + ", arguments:" + GetArgumentsString() + "]";
}

GraphMLXMLList EdgeCall::GraphMLAttributes() const {
  GraphMLXMLList attrs;
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefEdgeType)
      ->ToValue("call"));
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefMethodName)
      ->ToValue(method_));
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefCallArgs)
      ->ToValue(GetArgumentsString()));
  return attrs;
}

}  // namespace brave_page_graph
