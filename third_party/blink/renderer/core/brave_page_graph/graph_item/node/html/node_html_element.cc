/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/html/node_html_element.h"

#include <algorithm>
#include <sstream>

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/attribute/edge_attribute_delete.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/attribute/edge_attribute_set.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/edge_document.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/event_listener/edge_event_listener.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/event_listener/edge_event_listener_add.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/event_listener/edge_event_listener_remove.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/node/edge_node_create.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/node/edge_node_insert.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/node/edge_node_remove.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/request/edge_request_complete.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/request/edge_request_error.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/request/edge_request_start.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_actor.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/core/dom/dom_node_ids.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder_stream.h"

using ::blink::DOMNodeId;
using ::blink::DynamicTo;

namespace brave_page_graph {

NodeHTMLElement::NodeHTMLElement(GraphItemContext* context,
                                 const DOMNodeId dom_node_id,
                                 const String& tag_name)
    : NodeHTML(context, dom_node_id), tag_name_(tag_name) {}

NodeHTMLElement::~NodeHTMLElement() = default;

ItemName NodeHTMLElement::GetItemName() const {
  return "HTML element";
}

ItemDesc NodeHTMLElement::GetItemDesc() const {
  StringBuilder ts;
  ts << NodeHTML::GetItemDesc();

  ts << " [" << tag_name_;
  if (auto attribute = attributes_.find("id"); attribute != attributes_.end()) {
    ts << " id: " << attribute->value;
  }

  if (auto attribute = attributes_.find("class");
      attribute != attributes_.end()) {
    ts << " class: " << attribute->value;
  }
  ts << "]";

  return ts.ReleaseString();
}

void NodeHTMLElement::AddGraphMLTag(xmlDocPtr doc,
                                    xmlNodePtr parent_node) const {
  NodeHTML::AddGraphMLTag(doc, parent_node);

  for (NodeHTML* child_node : child_nodes_) {
    EdgeDocument document_edge(GetContext(), const_cast<NodeHTMLElement*>(this),
                               child_node);
    document_edge.AddGraphMLTag(doc, parent_node);
  }

  // For each event listener, draw an edge from the listener script to the DOM
  // node to which it's attached.
  for (auto& event_listener : event_listeners_) {
    const EventListenerId listener_id = event_listener.key;
    const String& event_type = event_listener.value->GetEventType();
    NodeActor* listener_node = event_listener.value->GetListenerNode();

    EdgeEventListener event_listener_edge(
        GetContext(), const_cast<NodeHTMLElement*>(this), listener_node,
        event_type, listener_id);
    event_listener_edge.AddGraphMLTag(doc, parent_node);
  }
}

void NodeHTMLElement::AddGraphMLAttributes(xmlDocPtr doc,
                                           xmlNodePtr parent_node) const {
  NodeHTML::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefNodeTag)
      ->AddValueNode(doc, parent_node, TagName());
}

void NodeHTMLElement::PlaceChildNodeAfterSiblingNode(NodeHTML* child,
                                                     NodeHTML* sibling) {
  // If this node has no current children, then this is easy, just add
  // the provided child as the only child.
  if (child_nodes_.size() == 0) {
    child_nodes_.push_back(child);
    return;
  }

  // Or, if sibling is null, then insert the child in the first position
  // in the child nodes.
  if (sibling == nullptr) {
    child_nodes_.insert(0, child);
    return;
  }

  // Otherwise, figure out where the sibling is in the child node set.
  const auto sib_pos = child_nodes_.Find(sibling);
  CHECK_NE(sib_pos, WTF::kNotFound);
  child_nodes_.insert(sib_pos + 1, child);
}

void NodeHTMLElement::RemoveChildNode(NodeHTML* child_node) {
  const auto child_pos = child_nodes_.Find(child_node);
  CHECK_NE(child_pos, WTF::kNotFound);
  child_nodes_.EraseAt(child_pos);
}

void NodeHTMLElement::MarkDeleted() {
  NodeHTML::MarkDeleted();
  for (NodeHTML* child_node : child_nodes_) {
    child_node->MarkDeleted();
  }
}

void NodeHTMLElement::AddInEdge(const GraphEdge* in_edge) {
  NodeHTML::AddInEdge(in_edge);
  if (const EdgeEventListenerAdd* add_event_listener_in_edge =
          DynamicTo<EdgeEventListenerAdd>(in_edge)) {
    event_listeners_.insert(add_event_listener_in_edge->GetListenerId(),
                            add_event_listener_in_edge);
  } else if (const EdgeEventListenerRemove* remove_event_listener_in_edge =
                 DynamicTo<EdgeEventListenerRemove>(in_edge)) {
    event_listeners_.erase(remove_event_listener_in_edge->GetListenerId());
  } else if (DynamicTo<EdgeNodeRemove>(in_edge)) {
    // Special case for when something (script) is removing an HTML element
    // from the DOM.  Update the parallel HTML context by removing the pointer
    // to the parent element.
    if (GetParentNode()) {
      GetParentNode()->RemoveChildNode(this);
    }
    SetParentNode(nullptr);
  } else if (const EdgeNodeInsert* insert_node_in_edge =
                 DynamicTo<EdgeNodeInsert>(in_edge)) {
    SetParentNode(insert_node_in_edge->GetParentNode());

    // Parent node will be nullptr if this is the root of a document, or a
    // subtree.
    if (GetParentNode()) {
      GetParentNode()->PlaceChildNodeAfterSiblingNode(
          this, insert_node_in_edge->GetPriorSiblingNode());
    }
  } else if (const EdgeAttributeSet* set_attribute_in_edge =
                 DynamicTo<EdgeAttributeSet>(in_edge)) {
    if (set_attribute_in_edge->IsStyle()) {
      inline_styles_.insert(set_attribute_in_edge->GetName(),
                            set_attribute_in_edge->GetValue());
    } else {
      attributes_.insert(set_attribute_in_edge->GetName(),
                         set_attribute_in_edge->GetValue());
    }
  } else if (const EdgeAttributeDelete* delete_attribute_in_edge =
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
