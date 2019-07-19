/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_AD_FILTER_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_AD_FILTER_H_

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_filter.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class PageGraph;

class NodeAdFilter : public NodeFilter {
friend class PageGraph;
 public:
  NodeAdFilter() = delete;
  ~NodeAdFilter() override;

  ItemName GetItemName() const override;
  bool IsNodeActor() const override { return false; }

  const std::string& GetRule() const;

 protected:
  NodeAdFilter(PageGraph* const graph, const std::string& rule);

  ItemDesc GetDescBody() const override;
  GraphMLXMLList GraphMLAttributes() const override;

 private:
  const std::string rule_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_AD_FILTER_H_
