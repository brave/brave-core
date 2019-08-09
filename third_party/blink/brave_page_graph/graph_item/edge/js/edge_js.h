/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_JS_EDGE_JS_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_JS_EDGE_JS_H_

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"

namespace brave_page_graph {

class Node;
class PageGraph;

class EdgeJS : public Edge {
friend class PageGraph;
 public:
  EdgeJS() = delete;
  ~EdgeJS() override;

  GraphMLXMLList GetGraphMLAttributes() const override;

  virtual const MethodName& GetMethodName() const = 0;
  bool IsEdgeJS() const override;
  virtual bool IsEdgeJSCall() const;
  virtual bool IsEdgeJSResult() const;

 protected:
  EdgeJS(PageGraph* const graph, Node* const out_node,
    Node* const in_node);
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeJS> {
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return edge.IsEdgeJS();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeJS>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_JS_EDGE_JS_H_
