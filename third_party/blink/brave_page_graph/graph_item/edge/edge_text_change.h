/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_TEXT_CHANGE_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_TEXT_CHANGE_H_

#include <string>

#include "third_party/blink/renderer/platform/wtf/casting.h"

#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"

namespace brave_page_graph {

class NodeHTMLText;
class NodeScript;
class PageGraph;

class EdgeTextChange : public Edge {
friend class PageGraph;
 public:
  EdgeTextChange() = delete;

  const std::string& GetText() const { return text_; }

  ItemName GetItemName() const override;
  ItemName GetItemDesc() const override;

  GraphMLXMLList GetGraphMLAttributes() const override;

  bool IsEdgeTextChange() const override;

 protected:
  EdgeTextChange(PageGraph* const graph, NodeScript* const out_node,
    NodeHTMLText* const in_node, const std::string& text);

 private:
  const std::string text_;
};

}  // namespace brave_page_graph

namespace blink {

template <>
struct DowncastTraits<brave_page_graph::EdgeTextChange> {
  static bool AllowFrom(const brave_page_graph::Edge& edge) {
    return edge.IsEdgeTextChange();
  }
  static bool AllowFrom(const brave_page_graph::GraphItem& graph_item) {
    return IsA<brave_page_graph::EdgeTextChange>(
        DynamicTo<brave_page_graph::Edge>(graph_item));
  }
};

}  // namespace blink

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_TEXT_CHANGE_H_
