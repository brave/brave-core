/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_JS_NODE_JS_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_JS_NODE_JS_H_

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/graph_node.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace brave_page_graph {

class NodeJS : public GraphNode {
 public:
  explicit NodeJS(GraphItemContext* context);
  ~NodeJS() override;

  virtual const MethodName& GetMethodName() const = 0;

  bool IsNodeJS() const override;

  virtual bool IsNodeJSBuiltin() const;
  virtual bool IsNodeJSWebAPI() const;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeJS> {
  static bool AllowFrom(const brave_page_graph::GraphNode& node) {
    return node.IsNodeJS();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeJS>(
        DynamicTo<brave_page_graph::GraphNode>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_JS_NODE_JS_H_
