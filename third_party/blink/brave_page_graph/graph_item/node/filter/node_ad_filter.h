/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_FILTER_NODE_AD_FILTER_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_FILTER_NODE_AD_FILTER_H_

#include <string>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/filter/node_filter.h"

namespace brave_page_graph {

class PageGraph;

class NodeAdFilter : public NodeFilter {
friend class PageGraph;
 public:
  NodeAdFilter() = delete;
  ~NodeAdFilter() override;

  const std::string& GetRule() const { return rule_; }

  ItemName GetItemName() const override;
  ItemDesc GetItemDesc() const override;

  GraphMLXMLList GetGraphMLAttributes() const override;

  bool IsNodeAdFilter() const override;

 protected:
  NodeAdFilter(PageGraph* const graph, const std::string& rule);

 private:
  const std::string rule_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeAdFilter> {
  static bool AllowFrom(const brave_page_graph::NodeFilter& filter_node) {
    return filter_node.IsNodeAdFilter();
  }
  static bool AllowFrom(const brave_page_graph::Node& node) {
    return IsA<brave_page_graph::NodeAdFilter>(
        DynamicTo<brave_page_graph::NodeFilter>(node));
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeAdFilter>(
        DynamicTo<brave_page_graph::Node>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_FILTER_NODE_AD_FILTER_H_
