/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/storage/edge_storage_read_result.h"

#include "base/check.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/actor/node_actor.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/storage/node_storage.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder_stream.h"

namespace brave_page_graph {

EdgeStorageReadResult::EdgeStorageReadResult(GraphItemContext* context,
                                             NodeStorage* out_node,
                                             NodeActor* in_node,
                                             const FrameId& frame_id,
                                             const String& key,
                                             const blink::PageGraphValue& value)
    : EdgeStorage(context, out_node, in_node, frame_id, key),
      value_(blink::PageGraphValueToString(value)) {
  CHECK(!in_node->IsNodeParser());
}

EdgeStorageReadResult::~EdgeStorageReadResult() = default;

ItemName EdgeStorageReadResult::GetItemName() const {
  return "storage read result";
}

ItemDesc EdgeStorageReadResult::GetItemDesc() const {
  StringBuilder ts;
  ts << EdgeStorage::GetItemDesc() << " [value: " << value_ << "]";
  return ts.ReleaseString();
}

void EdgeStorageReadResult::AddGraphMLAttributes(xmlDocPtr doc,
                                                 xmlNodePtr parent_node) const {
  EdgeStorage::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefValue)
      ->AddValueNode(doc, parent_node, value_);
}

bool EdgeStorageReadResult::IsEdgeStorageReadResult() const {
  return true;
}

}  // namespace brave_page_graph
