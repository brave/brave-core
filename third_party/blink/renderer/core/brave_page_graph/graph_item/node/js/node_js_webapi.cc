/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/js/node_js_webapi.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder_stream.h"

namespace brave_page_graph {

NodeJSWebAPI::NodeJSWebAPI(GraphItemContext* context, const MethodName method)
    : NodeJS(context), method_name_(method) {}

NodeJSWebAPI::~NodeJSWebAPI() = default;

const MethodName& NodeJSWebAPI::GetMethodName() const {
  return method_name_;
}

ItemName NodeJSWebAPI::GetItemName() const {
  return "web API";
}

ItemDesc NodeJSWebAPI::GetItemDesc() const {
  StringBuilder ts;
  ts << GraphNode::GetItemDesc() << " [" << method_name_ << "]";
  return ts.ReleaseString();
}

void NodeJSWebAPI::AddGraphMLAttributes(xmlDocPtr doc,
                                        xmlNodePtr parent_node) const {
  NodeJS::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefMethodName)
      ->AddValueNode(doc, parent_node, method_name_);
}

bool NodeJSWebAPI::IsNodeJSWebAPI() const {
  return true;
}

}  // namespace brave_page_graph
