/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html_element.h"
#include <algorithm>
#include <sstream>
#include <string>
#include "base/logging.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_attribute_delete.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_attribute_set.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_event_listener.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_event_listener_add.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_event_listener_remove.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_html.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_create.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_delete.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_insert.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_node_remove.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_complete.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_error.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_start.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_actor.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::blink::DOMNodeId;
using ::std::find;
using ::std::string;
using ::std::stringstream;
using ::std::to_string;

namespace brave_page_graph {

NodeHTMLElement::NodeHTMLElement(PageGraph* const graph,
    const DOMNodeId node_id, const string& tag_name) :
      NodeHTML(graph, node_id),
      tag_name_(tag_name) {}

NodeHTMLElement::~NodeHTMLElement() {}

ItemName NodeHTMLElement::GetItemName() const {
  return "HTML element #" + to_string(id_);
}

const string& NodeHTMLElement::TagName() const {
  return tag_name_;
}

bool NodeHTMLElement::HasAttribute(const std::string& key) const {
  return current_attributes_.count(key) == 1;
}

const std::string& NodeHTMLElement::GetAttribute(const std::string& key) const {
  return current_attributes_.at(key);
}

void NodeHTMLElement::AddInEdge(const EdgeEventListenerAdd* const edge) {
  event_listeners_.emplace(edge->GetListenerId(),
      EventListener(edge->GetEventType(), edge->GetListenerScriptId()));
  Node::AddInEdge(edge);
}

void NodeHTMLElement::AddInEdge(const EdgeEventListenerRemove* const edge) {
  event_listeners_.erase(edge->GetListenerId());
  Node::AddInEdge(edge);
}

// Special case for when something (script) is removing an HTML element
// from the DOM.  Update the parallel HTML graph by removing the pointer
// to the parent element.
void NodeHTMLElement::AddInEdge(const EdgeNodeRemove* const edge) {
  if (parent_node_ != nullptr) {
    parent_node_->RemoveChildNode(this);
  }
  parent_node_ = nullptr;
  Node::AddInEdge(edge);
}

void NodeHTMLElement::AddInEdge(const EdgeNodeInsert* const edge) {
  parent_node_ = edge->GetParentNode();
  // Parent node will be nullptr if this is the root of a document, or a
  // subtree.
  if (parent_node_ != nullptr) {
    parent_node_->PlaceChildNodeAfterSiblingNode(this,
      edge->GetPriorSiblingNode());
  }
  Node::AddInEdge(edge);
}

void NodeHTMLElement::AddInEdge(const EdgeNodeDelete* const edge) {
  MarkNodeDeleted();
  Node::AddInEdge(edge);
}

void NodeHTMLElement::AddInEdge(const EdgeAttributeSet* const edge) {
  if (edge->GetIsStyle()) {
    current_inline_styles_.emplace(edge->GetAttributeName(),
      edge->AttributeValue());
  } else {
    current_attributes_.emplace(edge->GetAttributeName(),
      edge->AttributeValue());
  }
  Node::AddInEdge(edge);
}

void NodeHTMLElement::AddInEdge(const EdgeAttributeDelete* const edge) {
  if (edge->GetIsStyle()) {
    current_inline_styles_.erase(edge->GetAttributeName());
  } else {
    current_attributes_.erase(edge->GetAttributeName());
  }
  Node::AddInEdge(edge);
}

const HTMLNodeList& NodeHTMLElement::ChildNodes() const {
  return child_nodes_;
}

GraphMLXML NodeHTMLElement::GetGraphMLTag() const {
  stringstream builder;
  builder << Node::GetGraphMLTag();

  for (NodeHTML* const child_node : child_nodes_) {
    EdgeHTML html_edge(this, child_node);
    builder << html_edge.GetGraphMLTag();
  }

  // For each event listener, draw an edge from the listener script to the DOM
  // node to which it's attached.
  for (auto& event_listener : event_listeners_) {
    const EventListenerId listener_id = event_listener.first;
    const string& event_type = event_listener.second.event_type;
    NodeActor* const listener_node = graph_->GetNodeActorForScriptId(
        event_listener.second.listener_script_id);

    EdgeEventListener event_listener_edge(this, listener_node, event_type,
                                          listener_id);
    builder << event_listener_edge.GetGraphMLTag();
  }

  return builder.str();
}

ItemDesc NodeHTMLElement::GetDescBody() const {
  stringstream string_builder;
  string_builder << GetItemName();
  string_builder << " (";
  string_builder << tag_name_;
  for (const auto& attr : current_attributes_) {
    const auto& key = attr.first;
    const auto& value = attr.second;
    if (key == "id") {
      string_builder << "#" << value;
    } else if (key == "class") {
      std::istringstream classNames(value);
      std::istream_iterator<std::string> className{classNames};
      for (; className != std::istream_iterator<std::string>(); ++className) {
        string_builder << "." << *className;
      }
    }
  }
  string_builder << ")";
  return string_builder.str();
}

void NodeHTMLElement::MarkNodeDeleted() {
  LOG_ASSERT(is_deleted_ == false);
  is_deleted_ = true;
  for (NodeHTML* node : child_nodes_) {
    node->MarkNodeDeleted();
  }
}

void NodeHTMLElement::PlaceChildNodeAfterSiblingNode(NodeHTML* const child,
    NodeHTML* const sibling) {
  // If this node has no current children, then this is easy, just add
  // the provided child as the only child.
  if (child_nodes_.size() == 0) {
    LOG_ASSERT(sibling == nullptr);
    child_nodes_.push_back(child);
    return;
  }

  // Or, if sibling is null, then insert the child in the first position
  // in the child nodes.
  if (sibling == nullptr) {
    child_nodes_.insert(child_nodes_.begin(), child);
    return;
  }

  // Otherwise, figure out where the sibling is in the child node set.
  const auto sib_pos = find(child_nodes_.begin(), child_nodes_.end(), sibling);
  LOG_ASSERT(sib_pos != child_nodes_.end());
  child_nodes_.insert(sib_pos + 1, child);
}

void NodeHTMLElement::RemoveChildNode(NodeHTML* const child_node) {
  const auto child_pos = find(child_nodes_.begin(), child_nodes_.end(),
    child_node);
  LOG_ASSERT(child_pos != child_nodes_.end());
  child_nodes_.erase(child_pos);
}

GraphMLXMLList NodeHTMLElement::GraphMLAttributes() const {
  GraphMLXMLList attrs = NodeHTML::GraphMLAttributes();
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefNodeType)
      ->ToValue("html node"));
  attrs.push_back(GraphMLAttrDefForType(kGraphMLAttrDefNodeTag)
      ->ToValue(TagName()));
  return attrs;
}

}  // namespace brave_page_graph
