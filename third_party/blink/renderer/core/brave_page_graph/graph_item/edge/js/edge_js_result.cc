/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/js/edge_js_result.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_script.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/js/node_js.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"
#include "third_party/blink/renderer/platform/wtf/text/text_stream.h"

using ::blink::To;

namespace brave_page_graph {

EdgeJSResult::EdgeJSResult(GraphItemContext* context,
                           NodeJS* out_node,
                           NodeScript* in_node,
                           const String& result)
    : EdgeJS(context, out_node, in_node), result_(result) {}

EdgeJSResult::~EdgeJSResult() = default;

ItemName EdgeJSResult::GetItemName() const {
  return "js result";
}

ItemDesc EdgeJSResult::GetItemDesc() const {
  WTF::TextStream ts;
  ts << GetItemName() << " [result: " << result_ << "]";
  return ts.Release();
}

void EdgeJSResult::AddGraphMLAttributes(xmlDocPtr doc,
                                        xmlNodePtr parent_node) const {
  EdgeJS::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefValue)
      ->AddValueNode(doc, parent_node, result_);
}

const String& EdgeJSResult::GetResult() const {
  return result_;
}

const MethodName& EdgeJSResult::GetMethodName() const {
  CHECK(GetOutNode()->IsNodeJS());
  return To<NodeJS>(GetOutNode())->GetMethodName();
}

bool EdgeJSResult::IsEdgeJSResult() const {
  return true;
}

}  // namespace brave_page_graph
