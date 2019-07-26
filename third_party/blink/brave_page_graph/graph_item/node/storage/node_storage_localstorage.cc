/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/storage/node_storage_localstorage.h"

#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

NodeStorageLocalStorage::NodeStorageLocalStorage(PageGraph* const graph) :
      NodeStorage(graph) {}

NodeStorageLocalStorage::~NodeStorageLocalStorage() {}

ItemName NodeStorageLocalStorage::GetItemName() const {
  return "local storage";
}

bool NodeStorageLocalStorage::IsNodeStorageLocalStorage() const {
  return true;
}

}  // namespace brave_page_graph
