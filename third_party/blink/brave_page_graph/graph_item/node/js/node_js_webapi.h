/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_JS_NODE_JS_WEBAPI_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_JS_NODE_JS_WEBAPI_H_

#include <string>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/js/node_js.h"

namespace brave_page_graph {

class PageGraph;

class NodeJSWebAPI final : public NodeJS {
friend class PageGraph;
 public:
  NodeJSWebAPI() = delete;
  ~NodeJSWebAPI() override;

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  GraphMLXMLList GetGraphMLAttributes() const override;

  const MethodName& GetMethodName() const override;
  bool IsNodeJSWebAPI() const override;

 protected:
  NodeJSWebAPI(PageGraph* const graph, const MethodName method);

  const MethodName method_name_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeJSWebAPI> {
  static bool AllowFrom(const brave_page_graph::Node& node) {
    return node.IsNodeJSWebAPI();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeJSWebAPI>(
        DynamicTo<brave_page_graph::Node>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_JS_NODE_JS_WEBAPI_H_
