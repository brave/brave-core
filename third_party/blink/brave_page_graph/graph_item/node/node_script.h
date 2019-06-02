/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_SCRIPT_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_SCRIPT_H_

#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_actor.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_requester.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class PageGraph;

class NodeScript final : public NodeActor, virtual public NodeRequester {
friend class PageGraph;
 public:
  NodeScript() = delete;
  ~NodeScript() override;
  ItemName GetItemName() const override;
  ScriptId GetScriptId() const;
  ScriptType GetScriptType() const;
  bool IsScript() const override;
  bool IsInline() const;
  std::string GetUrl() const;

 protected:
  NodeScript(PageGraph* const graph, const ScriptId script_id,
    const ScriptType type);
  NodeScript(PageGraph* const graph, const ScriptId script_id,
    const ScriptType type, const std::string& url);
  ItemDesc GetDescBody() const override;
  GraphMLXMLList GraphMLAttributes() const override;

  const ScriptId script_id_;
  const ScriptType type_;
  const std::string url_;
  const bool is_inline_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_SCRIPT_H_
