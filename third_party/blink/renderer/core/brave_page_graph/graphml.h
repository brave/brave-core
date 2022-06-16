/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPHML_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPHML_H_

#include <libxml/tree.h>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/time/time.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"

namespace brave_page_graph {

class GraphMLAttr {
 public:
  GraphMLAttr(const GraphMLAttrForType for_value,
              const std::string& name,
              const GraphMLAttrType type = kGraphMLAttrTypeString);

  GraphMLId GetGraphMLId() const;
  void AddDefinitionNode(xmlNodePtr parent_node) const;
  void AddValueNode(xmlDocPtr doc,
                    xmlNodePtr parent_node,
                    const char* value) const;
  void AddValueNode(xmlDocPtr doc,
                    xmlNodePtr parent_node,
                    const std::string& value) const;
  void AddValueNode(xmlDocPtr doc,
                    xmlNodePtr parent_node,
                    const int value) const;
  void AddValueNode(xmlDocPtr doc,
                    xmlNodePtr parent_node,
                    const bool value) const;
  void AddValueNode(xmlDocPtr doc,
                    xmlNodePtr parent_node,
                    const int64_t value) const;
  void AddValueNode(xmlDocPtr doc,
                    xmlNodePtr parent_node,
                    const uint64_t value) const;
  void AddValueNode(xmlDocPtr doc,
                    xmlNodePtr parent_node,
                    const double value) const;
  void AddValueNode(xmlDocPtr doc,
                    xmlNodePtr parent_node,
                    const base::TimeDelta value) const;

 protected:
  const uint64_t id_;
  const GraphMLAttrForType for_;
  const std::string name_;
  const GraphMLAttrType type_;
};

using GraphMLAttrs = base::flat_map<GraphMLAttrDef, raw_ptr<const GraphMLAttr>>;
const GraphMLAttrs& GetGraphMLAttrs();
const GraphMLAttr* GraphMLAttrDefForType(const GraphMLAttrDef type);

}  // namespace brave_page_graph

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPHML_H_
