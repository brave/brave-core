/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/js/node_js_builtin.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder_stream.h"

namespace brave_page_graph {

NodeJSBuiltin::NodeJSBuiltin(GraphItemContext* context,
                             const MethodName& builtin)
    : NodeJS(context), builtin_(builtin) {}

NodeJSBuiltin::~NodeJSBuiltin() = default;

const MethodName& NodeJSBuiltin::GetMethodName() const {
  return builtin_;
}

ItemName NodeJSBuiltin::GetItemName() const {
  return "JS builtin";
}

ItemDesc NodeJSBuiltin::GetItemDesc() const {
  StringBuilder ts;
  ts << GraphNode::GetItemDesc() << " [" << builtin_ << "]";
  return ts.ReleaseString();
}

void NodeJSBuiltin::AddGraphMLAttributes(xmlDocPtr doc,
                                         xmlNodePtr parent_node) const {
  NodeJS::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefMethodName)
      ->AddValueNode(doc, parent_node, builtin_);
}

bool NodeJSBuiltin::IsNodeJSBuiltin() const {
  return true;
}

}  // namespace brave_page_graph
