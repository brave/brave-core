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
 public:
  GraphItem() = delete;
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
  GraphItem(PageGraph* const graph);
  virtual GraphMLXMLList GraphMLAttributes() const;

  PageGraph* const graph_;
  const PageGraphId id_; 
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_GRAPH_ITEM_GRAPH_ITEM_H_
