/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_WEBAPI_EDGE_WEBAPI_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_WEBAPI_EDGE_WEBAPI_H_

#include <string>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"

namespace brave_page_graph {

class Node;
class NodeActor;
class NodeWebAPI;
class PageGraph;

class EdgeWebAPI : public Edge {
friend class PageGraph;
 public:
  EdgeWebAPI() = delete;
  ~EdgeWebAPI() override;

  const std::string& GetMethod() { return method_; }

  ItemDesc GetItemDesc() const override;

  GraphMLXMLList GetGraphMLAttributes() const override;

  bool IsEdgeWebAPI() const override;

  virtual bool IsEdgeWebAPICall() const;
  virtual bool IsEdgeWebAPIResult() const;

 protected:
  EdgeWebAPI(PageGraph* const graph, Node* const out_node,
    Node* const in_node, const std::string& method);

 private:
  const std::string method_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeWebAPI> {
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return edge.IsEdgeWebAPI();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeWebAPI>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_WEBAPI_EDGE_WEBAPI_H_
