/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_GRAPH_ITEM_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_GRAPH_ITEM_H_

#include <ctime>
#include <chrono>
#include <libxml/tree.h>

#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class PageGraph;

class GraphItem {
// Needed for ad-hoc id generation during GraphML export.
friend class PageGraph;
 public:
  virtual ~GraphItem();

  PageGraphId GetId() const { return id_; }
  ::std::chrono::milliseconds GetTimestamp() const { return time_; }

  virtual ItemName GetItemName() const = 0;
  virtual ItemDesc GetItemDesc() const;

  virtual GraphMLId GetGraphMLId() const = 0;
  virtual void AddGraphMLTag(xmlDocPtr doc, xmlNodePtr parent_node) const = 0;
  virtual void AddGraphMLAttributes(xmlDocPtr doc,
      xmlNodePtr parent_node) const;

  virtual bool IsEdge() const;
  virtual bool IsNode() const;

  ::std::chrono::milliseconds GetMicroSecSincePageStart() const;

 protected:
  GraphItem(PageGraph* const graph);
  // For use ONLY with items generated ad-hoc during GraphML export.
  GraphItem();

  PageGraph* GetGraph() const { return graph_; }

 protected:
  static void StartGraphMLExport(PageGraphId id_counter);

  const PageGraphId id_;
  const ::std::chrono::milliseconds time_;
  PageGraph* const graph_ = nullptr;
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_GRAPH_ITEM_H_
