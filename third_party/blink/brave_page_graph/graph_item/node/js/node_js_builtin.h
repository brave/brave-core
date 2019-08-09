/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_JS_NODE_JS_BUILTIN_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_JS_NODE_JS_BUILTIN_H_

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/js/node_js.h"

namespace brave_page_graph {

class PageGraph;

class NodeJSBuiltIn final : public NodeJS {
friend class PageGraph;
 public:
  NodeJSBuiltIn() = delete;
  ~NodeJSBuiltIn() override;

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  GraphMLXMLList GetGraphMLAttributes() const override;

  JSBuiltIn GetBuiltIn() const;
  const MethodName& GetMethodName() const override;
  bool IsNodeJSBuiltIn() const override;

 protected:
  NodeJSBuiltIn(PageGraph* const graph, const JSBuiltIn builtin);

  const JSBuiltIn built_in_name_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeJSBuiltIn> {
  static bool AllowFrom(const brave_page_graph::Node& node) {
    return node.IsNodeJSBuiltIn();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeJSBuiltIn>(
        DynamicTo<brave_page_graph::Node>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_JS_NODE_JS_BUILTIN_H_
