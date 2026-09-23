/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_JS_NODE_JS_WEBAPI_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_JS_NODE_JS_WEBAPI_H_

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/js/node_js.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace brave_page_graph {

class NodeJSWebAPI final : public NodeJS {
 public:
  NodeJSWebAPI(GraphItemContext* context, const MethodName method);
  ~NodeJSWebAPI() override;

  const MethodName& GetMethodName() const override;

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  void AddGraphMLAttributes(xmlDocPtr doc,
                            xmlNodePtr parent_node) const override;

  bool IsNodeJSWebAPI() const override;

 private:
  const MethodName method_name_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeJSWebAPI> {
  static bool AllowFrom(const brave_page_graph::NodeJS& js_node) {
    return js_node.IsNodeJSWebAPI();
  }
  static bool AllowFrom(const brave_page_graph::GraphNode& node) {
    return IsA<brave_page_graph::NodeJSWebAPI>(
        DynamicTo<brave_page_graph::NodeJS>(node));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeJSWebAPI>(
        DynamicTo<brave_page_graph::GraphNode>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_JS_NODE_JS_WEBAPI_H_
