/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/js/edge_js_call.h"

#include <sstream>
#include <string>
#include <vector>
#include <libxml/tree.h>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"
#include "brave/third_party/blink/brave_page_graph/logging.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_script.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/js/node_js.h"

using ::blink::To;
using ::std::string;
using ::std::stringstream;
using ::std::vector;

namespace brave_page_graph {

string BuildArgumentsString(const vector<const string>& arguments) {
  stringstream builder;
  const size_t num_args = arguments.size();
  const size_t last_index = num_args - 1;
  for (size_t i = 0; i < num_args; i += 1) {
    if (i == last_index) {
      builder << arguments.at(i);
    } else {
      builder << arguments.at(i) << ", ";
    }
  }
  return builder.str();
}

EdgeJSCall::EdgeJSCall(PageGraph* const graph,
    NodeScript* const out_node, NodeJS* const in_node,
    const vector<const string>& arguments, const int script_position) :
      EdgeJS(graph, out_node, in_node),
      arguments_(arguments),
      script_position_(script_position) {}

EdgeJSCall::~EdgeJSCall() {}

const MethodName& EdgeJSCall::GetMethodName() const {
  PG_LOG_ASSERT(GetInNode()->IsNodeJS());
  return To<NodeJS>(GetInNode())->GetMethodName();
}

ItemName EdgeJSCall::GetItemName() const {
  return "js call";
}

ItemDesc EdgeJSCall::GetItemDesc() const {
  return GetItemName() +
      " [arguments: " + BuildArgumentsString(arguments_) + "]";
}

void EdgeJSCall::AddGraphMLAttributes(xmlDocPtr doc,
    xmlNodePtr parent_node) const {
  EdgeJS::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefCallArgs)
      ->AddValueNode(doc, parent_node, BuildArgumentsString(arguments_));
  GraphMLAttrDefForType(kGraphMLAttrDefScriptPosition)
      ->AddValueNode(doc, parent_node, script_position_);
}

bool EdgeJSCall::IsEdgeJSCall() const {
  return true;
}

}  // namespace brave_page_graph
