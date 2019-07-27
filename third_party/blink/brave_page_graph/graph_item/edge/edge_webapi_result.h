/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_WEBAPI_RESULT_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_WEBAPI_RESULT_H_

#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_webapi.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class Node;
class NodeScript;
class NodeWebAPI;
class PageGraph;

class EdgeWebAPIResult final : public EdgeWebAPI {
friend class PageGraph;
 public:
  EdgeWebAPIResult() = delete;
  ~EdgeWebAPIResult() override;
  ItemName GetItemName() const override;

 protected:
  EdgeWebAPIResult(PageGraph* const graph, NodeWebAPI* const out_node,
    NodeScript* const in_node, const MethodName& method,
    const std::string& result);
  ItemDesc GetDescBody() const override;
  GraphMLXMLList GraphMLAttributes() const override;

  const std::string result_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_EDGE_EDGE_WEBAPI_RESULT_H_
