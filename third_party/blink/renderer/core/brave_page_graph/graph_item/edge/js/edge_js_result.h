/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_JS_EDGE_JS_RESULT_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_JS_EDGE_JS_RESULT_H_

#include <string>

#include "brave/third_party/blink/renderer/core/brave_page_graph/blink_probe_types.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/js/edge_js.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace brave_page_graph {

class NodeScript;
class NodeJS;

class EdgeJSResult final : public EdgeJS {
 public:
  EdgeJSResult(GraphItemContext* context,
               NodeJS* out_node,
               NodeScript* in_node,
               const FrameId& frame_id,
               const blink::PageGraphValue& result);

  ~EdgeJSResult() override;

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  void AddGraphMLAttributes(xmlDocPtr doc,
                            xmlNodePtr parent_node) const override;

  const std::string& GetResult() const;
  const MethodName& GetMethodName() const override;
  bool IsEdgeJSResult() const override;

 private:
  const std::string result_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeJSResult> {
  static bool AllowFrom(const brave_page_graph::EdgeJS& edge) {
    return edge.IsEdgeJSResult();
  }
  static bool AllowFrom(const brave_page_graph::GraphEdge& edge) {
    return IsA<brave_page_graph::EdgeJSResult>(
        DynamicTo<brave_page_graph::EdgeJS>(edge));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeJSResult>(
        DynamicTo<brave_page_graph::GraphEdge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_JS_EDGE_JS_RESULT_H_
