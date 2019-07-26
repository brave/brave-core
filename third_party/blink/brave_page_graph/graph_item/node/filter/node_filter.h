/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_FILTER_NODE_FILTER_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_FILTER_NODE_FILTER_H_

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

namespace brave_page_graph {

class PageGraph;

class NodeFilter : public Node {
friend class PageGraph;
 public:
  NodeFilter() = delete;
  ~NodeFilter() override;

  bool IsNodeFilter() const override;

  virtual bool IsNodeAdFilter() const;
  virtual bool IsNodeFingerprintingFilter() const;
  virtual bool IsNodeTrackerFilter() const;

 protected:
  NodeFilter(PageGraph* const graph);
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::NodeFilter> {
  static bool AllowFrom(const brave_page_graph::Node& node) {
    return node.IsNodeFilter();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::NodeFilter>(
        DynamicTo<brave_page_graph::Node>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_FILTER_NODE_FILTER_H_
