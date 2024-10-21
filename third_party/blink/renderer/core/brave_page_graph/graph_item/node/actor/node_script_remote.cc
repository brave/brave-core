// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_script_remote.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder_stream.h"

namespace brave_page_graph {

NodeScriptRemote::NodeScriptRemote(GraphItemContext* context,
                                   const ScriptId script_id)
    : NodeScript(context, script_id) {}

NodeScriptRemote::~NodeScriptRemote() = default;

ItemName NodeScriptRemote::GetItemName() const {
  return "remote script";
}

bool NodeScriptRemote::IsNodeScriptRemote() const {
  return true;
}

}  // namespace brave_page_graph
