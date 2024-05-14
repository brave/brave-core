/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPHML_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPHML_H_

#include <libxml/tree.h>

#include <string_view>

#include "base/containers/flat_map.h"
#include "base/time/time.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace brave_page_graph {

class GraphMLAttr {
 public:
  GraphMLAttr(const GraphMLAttrForType for_value,
              const String& name,
              const GraphMLAttrType type = kGraphMLAttrTypeString);

  const GraphMLId& GetGraphMLId() const;

  void AddDefinitionNode(xmlNodePtr parent_node) const;

  void AddValueNode(xmlDocPtr doc,
                    xmlNodePtr parent_node,
                    std::string_view value) const;
  void AddValueNode(xmlDocPtr doc,
                    xmlNodePtr parent_node,
                    const String& value) const;
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

 private:
  void AddValueNodeXmlChar(xmlDocPtr doc,
                           xmlNodePtr parent_node,
                           const xmlChar* value) const;

  const GraphMLAttrForType for_;
  const String name_;
  const GraphMLAttrType type_;
  const GraphMLId graphml_id_;
};

using GraphMLAttrs = base::flat_map<GraphMLAttrDef, const GraphMLAttr*>;
const GraphMLAttrs& GetGraphMLAttrs();
const GraphMLAttr* GraphMLAttrDefForType(const GraphMLAttrDef type);

}  // namespace brave_page_graph

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_GRAPHML_H_
