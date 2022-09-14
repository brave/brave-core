/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/storage/node_storage_cookiejar.h"

namespace brave_page_graph {

NodeStorageCookieJar::NodeStorageCookieJar(GraphItemContext* context)
    : NodeStorage(context) {}

NodeStorageCookieJar::~NodeStorageCookieJar() = default;

ItemName NodeStorageCookieJar::GetItemName() const {
  return "cookie jar";
}

bool NodeStorageCookieJar::IsNodeStorageCookieJar() const {
  return true;
}

}  // namespace brave_page_graph
