/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/html/node_html_element.h"

#include <algorithm>
#include <sstream>
#include <string>
#include <libxml/tree.h>

#include "base/logging.h"

#include "third_party/blink/renderer/core/dom/dom_node_ids.h"

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_html.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/attribute/edge_attribute_delete.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/attribute/edge_attribute_set.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/event_listener/edge_event_listener.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/event_listener/edge_event_listener_add.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/event_listener/edge_event_listener_remove.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/node/edge_node_create.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/node/edge_node_insert.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/node/edge_node_remove.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_complete.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_error.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_start.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_actor.h"

using ::std::endl;
using ::std::find;
using ::std::istream_iterator;
using ::std::istringstream;
using ::std::string;
using ::std::stringstream;
using ::std::to_string;

using ::blink::DOMNodeId;
using ::blink::DynamicTo;

namespace brave_page_graph {

NodeHTMLElement::NodeHTMLElement(PageGraph* const graph,
    const DOMNodeId node_id, const string& tag_name) :
      NodeHTML(graph, node_id),
      tag_name_(tag_name) {}

NodeHTMLElement::~NodeHTMLElement() {}

ItemName NodeHTMLElement::GetItemName() const {
  return "HTML element";
}

ItemDesc NodeHTMLElement::GetItemDesc() const {
  stringstream builder;
  builder << NodeHTML::GetItemDesc();

  builder << " [" << tag_name_;
  if (attributes_.count("id") == 1) {
    builder << "#" << attributes_.at("id");
  }
  if (attributes_.count("class") == 1) {
    builder << "#" << attributes_.at("id");
    istringstream class_names(attributes_.at("class"));
    istream_iterator<std::string> class_name{class_names};
    for (; class_name != istream_iterator<string>(); ++class_name) {
      builder << "." << *class_name;
    }
  }
  builder << "]";

  return builder.str();
}

void NodeHTMLElement::AddGraphMLTag(xmlDocPtr doc,
    xmlNodePtr parent_node) const {
  NodeHTML::AddGraphMLTag(doc, parent_node);

  for (NodeHTML* const child_node : child_nodes_) {
    EdgeHTML html_edge(this, child_node);
    html_edge.AddGraphMLTag(doc, parent_node);
  }

  // For each event listener, draw an edge from the listener script to the DOM
  // node to which it's attached.
  for (auto& event_listener : event_listeners_) {
    const EventListenerId listener_id = event_listener.first;
    const string& event_type = event_listener.second.event_type;
    NodeActor* const listener_node = GetGraph()->GetNodeActorForScriptId(
        event_listener.second.listener_script_id);

    EdgeEventListener event_listener_edge(this, listener_node, event_type,
                                          listener_id);
    event_listener_edge.AddGraphMLTag(doc, parent_node);
  }
}

void NodeHTMLElement::AddGraphMLAttributes(xmlDocPtr doc,
    xmlNodePtr parent_node) const {
  NodeHTML::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefNodeTag)
      ->AddValueNode(doc, parent_node, TagName());
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

void NodeHTMLElement::MarkDeleted() {
  NodeHTML::MarkDeleted();
  for (NodeHTML* child_node : child_nodes_) {
    child_node->MarkDeleted();
  }
}

void NodeHTMLElement::AddInEdge(const Edge* const in_edge) {
  NodeHTML::AddInEdge(in_edge);
  if (const EdgeEventListenerAdd* const add_event_listener_in_edge =
          DynamicTo<EdgeEventListenerAdd>(in_edge)) {
    event_listeners_.emplace(add_event_listener_in_edge->GetListenerId(),
        EventListener(add_event_listener_in_edge->GetEventType(),
                      add_event_listener_in_edge->GetListenerScriptId()));
  } else if (
      const EdgeEventListenerRemove* const remove_event_listener_in_edge =
          DynamicTo<EdgeEventListenerRemove>(in_edge)) {
    event_listeners_.erase(remove_event_listener_in_edge->GetListenerId());
  } else if (const EdgeNodeRemove* const remove_node_in_edge =
                 DynamicTo<EdgeNodeRemove>(in_edge)) {
    // Special case for when something (script) is removing an HTML element
    // from the DOM.  Update the parallel HTML graph by removing the pointer
    // to the parent element.
    if (GetParentNode()) {
      GetParentNode()->RemoveChildNode(this);
    }
    SetParentNode(nullptr);
  } else if (const EdgeNodeInsert* const insert_node_in_edge =
                 DynamicTo<EdgeNodeInsert>(in_edge)) {
    SetParentNode(insert_node_in_edge->GetParentNode());

    // Parent node will be nullptr if this is the root of a document, or a
    // subtree.
    if (GetParentNode()) {
      GetParentNode()->PlaceChildNodeAfterSiblingNode(this,
          insert_node_in_edge->GetPriorSiblingNode());
    }
  } else if (const EdgeAttributeSet* const set_attribute_in_edge =
                 DynamicTo<EdgeAttributeSet>(in_edge)) {
    if (set_attribute_in_edge->IsStyle()) {
      inline_styles_.emplace(set_attribute_in_edge->GetName(),
          set_attribute_in_edge->GetValue());
    } else {
      attributes_.emplace(set_attribute_in_edge->GetName(),
          set_attribute_in_edge->GetValue());
    }
  } else if (const EdgeAttributeDelete* const delete_attribute_in_edge =
                 DynamicTo<EdgeAttributeDelete>(in_edge)) {
    if (delete_attribute_in_edge->IsStyle()) {
      inline_styles_.erase(delete_attribute_in_edge->GetName());
    } else {
      attributes_.erase(delete_attribute_in_edge->GetName());
    }
  }
}

bool NodeHTMLElement::IsNodeHTMLElement() const {
  return true;
}

bool NodeHTMLElement::IsNodeDOMRoot() const {
  return false;
}

bool NodeHTMLElement::IsNodeFrameOwner() const {
  return false;
}

}  // namespace brave_page_graph
