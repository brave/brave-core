/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_FILTER_NODE_TRACKER_FILTER_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_FILTER_NODE_TRACKER_FILTER_H_

#include <string>

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/filter/node_filter.h"

namespace brave_page_graph {

class PageGraph;

class NodeTrackerFilter : public NodeFilter {
friend class PageGraph;
 public:
  NodeTrackerFilter() = delete;
  ~NodeTrackerFilter() override;

  const std::string& GetHost() const { return host_; }

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  GraphMLXMLList GetGraphMLAttributes() const override;

  bool IsNodeTrackerFilter() const override;

 protected:
  NodeTrackerFilter(PageGraph* const graph, const std::string& host);

 private:
  const std::string host_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeTrackerFilter> {
  static bool AllowFrom(const brave_page_graph::NodeFilter& filter_node) {
    return filter_node.IsNodeTrackerFilter();
  }
  static bool AllowFrom(const brave_page_graph::Node& node) {
    return IsA<brave_page_graph::NodeTrackerFilter>(
        DynamicTo<brave_page_graph::NodeFilter>(node));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeTrackerFilter>(
        DynamicTo<brave_page_graph::Node>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_FILTER_NODE_TRACKER_FILTER_H_
