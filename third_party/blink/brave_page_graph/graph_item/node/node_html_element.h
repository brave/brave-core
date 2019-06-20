/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_HTML_ELEMENT_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_HTML_ELEMENT_H_

#include <map>
#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class PageGraph;
class EdgeAttributeDelete;
class EdgeAttributeSet;
class EdgeNodeCreate;
class EdgeNodeDelete;
class EdgeNodeInsert;
class EdgeNodeRemove;
class EdgeRequestComplete;
class EdgeRequestError;
class EdgeRequestStart;
class NodeResource;

class NodeHTMLElement : public NodeHTML {
friend class PageGraph;
friend class NodeHTMLText;
friend class NodeHTML;
 public:
  NodeHTMLElement() = delete;
  ~NodeHTMLElement() override;
  ItemName GetItemName() const override;
  const std::string& TagName() const;

  using Node::AddInEdge;
  void AddInEdge(const EdgeNodeRemove* const edge);
  void AddInEdge(const EdgeNodeInsert* const edge);
  void AddInEdge(const EdgeNodeDelete* const edge);
  void AddInEdge(const EdgeAttributeSet* const edge);
  void AddInEdge(const EdgeAttributeDelete* const edge);
  void AddOutEdge(const EdgeAttributeDelete* const edge);

  using Node::AddOutEdge;

  const HTMLNodeList& ChildNodes() const;
  GraphMLXML GetGraphMLTag() const override;

 protected:
  NodeHTMLElement(PageGraph* const graph, const blink::DOMNodeId node_id,
    const std::string& tag_name);
  ItemDesc GetDescBody() const override;
  void MarkNodeDeleted() override;
  void PlaceChildNodeAfterSiblingNode(NodeHTML* const child,
    NodeHTML* const sibling);
  void RemoveChildNode(NodeHTML* const child_node);
  GraphMLXMLList GraphMLAttributes() const override;

  const std::string tag_name_;
  AttributeMap current_attributes_;
  AttributeMap current_inline_styles_;
  HTMLNodeList child_nodes_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_NODE_NODE_HTML_ELEMENT_H_
