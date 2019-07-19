/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_FINGERPRINTING_FILTER_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_FINGERPRINTING_FILTER_H_

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_filter.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class PageGraph;

class NodeFingerprintingFilter : public NodeFilter {
friend class PageGraph;
 public:
  NodeFingerprintingFilter() = delete;
  ~NodeFingerprintingFilter() override;

  ItemName GetItemName() const override;
  bool IsNodeActor() const override { return false; }

  const FingerprintingRule& GetRule() const;

 protected:
  NodeFingerprintingFilter(PageGraph* const graph,
                           const FingerprintingRule& rule);

  ItemDesc GetDescBody() const override;
  GraphMLXMLList GraphMLAttributes() const override;

 private:
  const FingerprintingRule rule_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_FINGERPRINTING_FILTER_H_
