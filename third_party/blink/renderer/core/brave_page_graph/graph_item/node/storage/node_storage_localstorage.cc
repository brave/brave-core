/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/storage/node_storage_localstorage.h"

namespace brave_page_graph {

NodeStorageLocalStorage::NodeStorageLocalStorage(GraphItemContext* context)
    : NodeStorage(context) {}

NodeStorageLocalStorage::~NodeStorageLocalStorage() = default;

ItemName NodeStorageLocalStorage::GetItemName() const {
  return "local storage";
}

bool NodeStorageLocalStorage::IsNodeStorageLocalStorage() const {
  return true;
}

}  // namespace brave_page_graph
