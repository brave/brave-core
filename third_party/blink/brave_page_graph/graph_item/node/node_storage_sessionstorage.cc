/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_storage_sessionstorage.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_storage.h"
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::to_string;

namespace brave_page_graph {

NodeStorageSessionStorage::NodeStorageSessionStorage(PageGraph* const graph) :
      NodeStorage(graph) {}

NodeStorageSessionStorage::~NodeStorageSessionStorage() {}

ItemName NodeStorageSessionStorage::GetItemName() const {
  return "session storage";
}

GraphMLXMLList NodeStorageSessionStorage::GraphMLAttributes() const {
  return {
    GraphMLAttrDefForType(kGraphMLAttrDefNodeType)
      ->ToValue("session storage")
  };
}

}  // namespace brave_page_graph
