/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPHML_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPHML_H_

#include <string>
#include <vector>
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class Edge;
class Node;
class NodeHTMLElement;
class PageGraph;

class GraphMLAttr {
 public:
  GraphMLAttr() = delete;
  GraphMLAttr(const GraphMLAttrForType for_value, const std::string& name);
  GraphMLAttr(const GraphMLAttrForType for_value, const std::string& name,
    const GraphMLAttrType type);
  GraphMLId GetGraphMLId() const;
  GraphMLXML ToDefinition() const;
  GraphMLXML ToValue(const char* value) const;
  GraphMLXML ToValue(const std::string& value) const;
  GraphMLXML ToValue(const int value) const;
  GraphMLXML ToValue(const bool value) const;
  GraphMLXML ToValue(const uint64_t value) const;

 protected:
  const uint64_t id_;
  const GraphMLAttrForType for_;
  const std::string name_;
  const GraphMLAttrType type_;
};

const GraphMLAttr* graphml_attr_def_for_type(const GraphMLAttrDef type) noexcept;
GraphMLXML graphml_attr_def_to_string(const GraphMLAttr attr_def) noexcept;
const std::vector<const GraphMLAttr* const>& get_graphml_attrs();

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPHML_H_
