/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_storage_cookiejar.h"
#include <string>
#include "brave/third_party/blink/brave_page_graph/graphml.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_storage.h"
#include "brave/third_party/blink/brave_page_graph/page_graph.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::to_string;

namespace brave_page_graph {

NodeStorageCookieJar::NodeStorageCookieJar(PageGraph* const graph) :
      NodeStorage(graph) {}

NodeStorageCookieJar::~NodeStorageCookieJar() {}

ItemName NodeStorageCookieJar::GetItemName() const {
  return "NodeStorageCookieJar#" + to_string(id_);
}

GraphMLXMLList NodeStorageCookieJar::GraphMLAttributes() const {
  return {
    graphml_attr_def_for_type(kGraphMLAttrDefNodeType)->ToValue("cookie jar")
  };
}

}  // namespace brave_page_graph
