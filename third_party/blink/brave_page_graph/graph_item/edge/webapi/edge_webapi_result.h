/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_WEBAPI_RESULT_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_WEBAPI_RESULT_H_

#include <string>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/webapi/edge_webapi.h"

namespace brave_page_graph {

class Node;
class NodeScript;
class NodeWebAPI;
class PageGraph;

class EdgeWebAPIResult final : public EdgeWebAPI {
friend class PageGraph;
 public:
  EdgeWebAPIResult() = delete;
  ~EdgeWebAPIResult() override;

  const std::string& GetResult() const { return result_; }

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  GraphMLXMLList GetGraphMLAttributes() const override;

  bool IsEdgeWebAPIResult() const override;

 protected:
  EdgeWebAPIResult(PageGraph* const graph, NodeWebAPI* const out_node,
    NodeScript* const in_node, const MethodName& method,
    const std::string& result);

 private:
  const std::string result_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeWebAPIResult> {
  static bool AllowFrom(const brave_page_graph::EdgeWebAPI& edge) {
    return edge.IsEdgeWebAPIResult();
  }
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return IsA<brave_page_graph::EdgeWebAPIResult>(
        DynamicTo<brave_page_graph::EdgeWebAPI>(edge));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeWebAPIResult>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_WEBAPI_RESULT_H_
