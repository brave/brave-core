/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/storage/node_storage.h"

namespace brave_page_graph {

NodeStorage::NodeStorage(GraphItemContext* context) : GraphNode(context) {}

bool NodeStorage::IsNodeStorage() const {
  return true;
}

bool NodeStorage::IsNodeStorageCookieJar() const {
  return false;
}

bool NodeStorage::IsNodeStorageLocalStorage() const {
  return false;
}

bool NodeStorage::IsNodeStorageSessionStorage() const {
  return false;
}

}  // namespace brave_page_graph
