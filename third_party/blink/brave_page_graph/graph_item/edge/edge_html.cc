/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge_html.h"
#include <ostream>
#include <sstream>
#include <string>
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/edge.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_html_element.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::string;
using ::std::stringstream;
using ::std::to_string;

namespace brave_page_graph {

EdgeHTML::EdgeHTML(PageGraph* const graph, NodeHTMLElement* const out_node,
    NodeHTML* const in_node) :
      Edge(graph, out_node, in_node) {}

EdgeHTML::EdgeHTML(const NodeHTMLElement* const out_node,
    NodeHTML* const in_node) :
      Edge(nullptr, const_cast<NodeHTMLElement*>(out_node), in_node) {}

EdgeHTML::~EdgeHTML() {}

ItemName EdgeHTML::GetItemName() const {
  return "EdgeHTML#" + to_string(id_);
}

GraphMLXML EdgeHTML::GetGraphMLTag() const {
  // graph_ will be null when EdgeHTML elements are being created only
  // for temporary GraphML export.  In all other cases graph_ will
  // point to the shared PageGraph instance/
  if (graph_ != nullptr) {
    return Edge::GetGraphMLTag();
  }

  // To ensure all tag ids are unique, dervie a graphml id based on
  // the parent and child DOMNodeIds, which will also make a unique combination. 
  const string graphml_id = to_string(out_node_->Id()) + "-" +
                            to_string(in_node_->Id());

  stringstream builder;
  builder << "<edge id=\"t" + graphml_id << "\" " <<
                      "source=\"" << out_node_->GetGraphMLId() << "\" " <<
                      "target=\"" << in_node_->GetGraphMLId() << "\">";

  for (const GraphMLXML& elm : GraphMLAttributes()) {
    builder << "\t" << elm;
  }
  builder << "</edge>";
  return builder.str();
}

GraphMLXMLList EdgeHTML::GraphMLAttributes() const {
  return {
    graphml_attr_def_for_type(kGraphMLAttrDefEdgeType)
      ->ToValue("structure")
  };
}

}  // namespace brave_page_graph
