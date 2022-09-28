/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_actor.h"

namespace brave_page_graph {

NodeActor::NodeActor(GraphItemContext* context) : GraphNode(context) {}

NodeActor::~NodeActor() = default;

bool NodeActor::IsNodeActor() const {
  return true;
}

bool NodeActor::IsNodeParser() const {
  return false;
}

bool NodeActor::IsNodeScript() const {
  return false;
}

}  // namespace brave_page_graph
