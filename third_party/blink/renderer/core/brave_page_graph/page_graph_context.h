/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_PAGE_GRAPH_CONTEXT_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_PAGE_GRAPH_CONTEXT_H_

#include <memory>
#include <type_traits>
#include <utility>

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/graph_item_context.h"

namespace brave_page_graph {

class GraphNode;
class GraphEdge;

class PageGraphContext : public GraphItemContext {
 public:
  virtual void AddGraphItem(std::unique_ptr<GraphItem> graph_item) = 0;

  template <typename T, typename... Args>
  T* AddNode(Args&&... args) {
    static_assert(std::is_base_of<GraphNode, T>::value,
                  "AddNode only for Nodes");
    T* node = new T(this, std::forward<Args>(args)...);
    AddGraphItem(std::unique_ptr<T>(node));
    return node;
  }

  template <typename T, typename... Args>
  T* AddEdge(Args&&... args) {
    static_assert(std::is_base_of<GraphEdge, T>::value,
                  "AddEdge only for Edges");
    T* edge = new T(this, std::forward<Args>(args)...);
    AddGraphItem(std::unique_ptr<T>(edge));
    return edge;
  }
};

}  // namespace brave_page_graph

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_PAGE_GRAPH_CONTEXT_H_
