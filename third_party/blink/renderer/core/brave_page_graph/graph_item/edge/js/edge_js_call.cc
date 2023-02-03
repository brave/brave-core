/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/js/edge_js_call.h"

#include <utility>

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_script.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/js/node_js.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/platform/wtf/text/text_stream.h"

using ::blink::To;

namespace brave_page_graph {

String BuildArgumentsString(const Vector<String>& arguments) {
  WTF::TextStream ts;
  const size_t num_args = arguments.size();
  const size_t last_index = num_args - 1;
  for (wtf_size_t i = 0; i < num_args; i += 1) {
    if (i == last_index) {
      ts << arguments.at(i);
    } else {
      ts << arguments.at(i) << ", ";
    }
  }
  return ts.Release();
}

EdgeJSCall::EdgeJSCall(GraphItemContext* context,
                       NodeScript* out_node,
                       NodeJS* in_node,
                       Vector<String> arguments,
                       const int script_position)
    : EdgeJS(context, out_node, in_node),
      arguments_(std::move(arguments)),
      script_position_(script_position) {}

EdgeJSCall::~EdgeJSCall() = default;

const MethodName& EdgeJSCall::GetMethodName() const {
  CHECK(GetInNode()->IsNodeJS());
  return To<NodeJS>(GetInNode())->GetMethodName();
}

ItemName EdgeJSCall::GetItemName() const {
  return "js call";
}

ItemDesc EdgeJSCall::GetItemDesc() const {
  WTF::TextStream ts;
  ts << GetItemName() << " [arguments: " << BuildArgumentsString(arguments_)
     << "]";
  return ts.Release();
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
