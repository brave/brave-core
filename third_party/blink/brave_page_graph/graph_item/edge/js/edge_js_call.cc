/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/js/edge_js_call.h"

#include <sstream>
#include <string>
#include <vector>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_script.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/js/node_js.h"

using ::blink::To;
using ::std::string;
using ::std::stringstream;
using ::std::vector;

namespace brave_page_graph {

EdgeJSCall::EdgeJSCall(PageGraph* const graph,
    NodeScript* const out_node, NodeJS* const in_node,
    const vector<const string>& arguments) :
      EdgeJS(graph, out_node, in_node),
      arguments_(arguments) {}

EdgeJSCall::~EdgeJSCall() {}

ItemName EdgeJSCall::GetItemName() const {
  return "js call";
}

ItemDesc EdgeJSCall::GetItemDesc() const {
  return GetItemName() + " [arguments: " + GetArgumentsString() + "]";
}

GraphMLXMLList EdgeJSCall::GetGraphMLAttributes() const {
  GraphMLXMLList attrs = EdgeJS::GetGraphMLAttributes();
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefCallArgs)
      ->ToValue(GetArgumentsString()));
  return attrs;
}

const vector<const string>& EdgeJSCall::GetArguments() const {
  return arguments_;
}

string EdgeJSCall::GetArgumentsString() const {
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

const MethodName& EdgeJSCall::GetMethodName() const {
  LOG_ASSERT(GetInNode()->IsNodeJS());
  return To<NodeJS>(GetInNode())->GetMethodName();
}

bool EdgeJSCall::IsEdgeJSCall() const {
  return true;
}

}  // namespace brave_page_graph
