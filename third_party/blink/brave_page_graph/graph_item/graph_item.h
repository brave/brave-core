/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_GRAPH_ITEM_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_GRAPH_ITEM_H_

#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class PageGraph;

class GraphItem {
// Needed for ad-hoc id generation during GraphML export.
friend class PageGraph;
 public:
  virtual ~GraphItem();
  virtual ItemDesc GetDesc() const;
  virtual ItemName GetItemName() const = 0;
  PageGraphId Id() const;

  virtual GraphMLId GetGraphMLId() const = 0;
  virtual GraphMLXML GetGraphMLTag() const = 0;

  virtual ItemDesc GetDescBody() const;
  virtual ItemDesc GetDescPrefix() const = 0;
  virtual ItemDesc GetDescSuffix() const = 0;

 protected:
  // For use ONLY with items generated ad-hoc during GraphML export.
  GraphItem();

  GraphItem(PageGraph* const graph);
  virtual GraphMLXMLList GraphMLAttributes() const;

  PageGraph* const graph_;
  const PageGraphId id_;

 private:
  static void StartGraphMLExport(PageGraphId id_counter);
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_GRAPH_ITEM_H_
