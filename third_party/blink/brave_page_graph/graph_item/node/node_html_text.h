/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_HTML_TEXT_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_HTML_TEXT_H_

#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class EdgeNodeCreate;
class EdgeNodeDelete;
class EdgeNodeInsert;
class EdgeNodeRemove;
class EdgeTextChange;
class PageGraph;

class NodeHTMLText final : public NodeHTML {
friend class PageGraph;
 public:
  NodeHTMLText() = delete;
  ~NodeHTMLText() override;
  ItemName GetItemName() const override;
  const std::string& Text() const;

  void AddInEdge(const EdgeNodeCreate* const edge);
  void AddInEdge(const EdgeNodeRemove* const edge);
  void AddInEdge(const EdgeNodeInsert* const edge);
  void AddInEdge(const EdgeNodeDelete* const edge);
  void AddInEdge(const EdgeTextChange* const edge);

  void AddOutEdge(const Edge* const edge) = delete;

 protected:
  NodeHTMLText(PageGraph* const graph, const blink::DOMNodeId node_id,
    const std::string& text);
  ItemDesc GetDescBody() const override;
  GraphMLXMLList GraphMLAttributes() const override;

  const std::string text_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_HTML_TEXT_H_
