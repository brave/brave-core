/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/binding/edge_binding_event.h"

#include <string>

#include "base/logging.h"

#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/actor/node_script.h"

#include "brave/third_party/blink/brave_page_graph/graph_item/node/binding/node_binding_event.h"

namespace brave_page_graph {

EdgeBindingEvent::EdgeBindingEvent(PageGraph* const graph,
    NodeScript* const out_node, NodeBindingEvent* const in_node,
    const int script_position) :
      Edge(graph, out_node, in_node),
      script_position_(script_position) {}

EdgeBindingEvent::~EdgeBindingEvent() {}

ItemName EdgeBindingEvent::GetItemName() const {
  return "binding event";
}

ItemDesc EdgeBindingEvent::GetItemDesc() const {
  return GetItemName();
}

void EdgeBindingEvent::AddGraphMLAttributes(xmlDocPtr doc,
    xmlNodePtr parent_node) const {
  Edge::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefScriptPosition)
      ->AddValueNode(doc, parent_node, script_position_);
}

bool EdgeBindingEvent::IsEdgeBindingEvent() const {
  return true;
}

}  // namespace brave_page_graph
