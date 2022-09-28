/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_JS_EDGE_JS_CALL_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_JS_EDGE_JS_CALL_H_

#include <string>
#include <vector>

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/js/edge_js.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace brave_page_graph {

class NodeScript;
class NodeJS;

class EdgeJSCall final : public EdgeJS {
 public:
  EdgeJSCall(GraphItemContext* context,
             NodeScript* out_node,
             NodeJS* in_node,
             const std::vector<std::string>& arguments,
             const int script_position);

  ~EdgeJSCall() override;

  const MethodName& GetMethodName() const override;

  const std::vector<std::string>& GetArguments() const { return arguments_; }

  int GetScriptPosition() const { return script_position_; }

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  void AddGraphMLAttributes(xmlDocPtr doc,
                            xmlNodePtr parent_node) const override;

  bool IsEdgeJSCall() const override;

 private:
  const std::vector<std::string> arguments_;
  const int script_position_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeJSCall> {
  static bool AllowFrom(const brave_page_graph::EdgeJS& edge) {
    return edge.IsEdgeJSCall();
  }
  static bool AllowFrom(const brave_page_graph::GraphEdge& edge) {
    return IsA<brave_page_graph::EdgeJSCall>(
        DynamicTo<brave_page_graph::EdgeJS>(edge));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeJSCall>(
        DynamicTo<brave_page_graph::GraphEdge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_JS_EDGE_JS_CALL_H_
