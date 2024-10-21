/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/request/edge_request_complete.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/node_resource.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graphml.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/utilities/response_metadata.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/wtf/text/string_builder_stream.h"

namespace brave_page_graph {

EdgeRequestComplete::EdgeRequestComplete(GraphItemContext* context,
                                         NodeResource* out_node,
                                         GraphNode* in_node,
                                         const InspectorId request_id,
                                         const FrameId& frame_id,
                                         const String& resource_type,
                                         const ResponseMetadata& metadata,
                                         const String& hash)
    : EdgeRequestResponse(context,
                          out_node,
                          in_node,
                          request_id,
                          frame_id,
                          kRequestStatusComplete,
                          metadata),
      resource_type_(resource_type),
      hash_(hash) {}

EdgeRequestComplete::~EdgeRequestComplete() = default;

ItemName EdgeRequestComplete::GetItemName() const {
  return "request complete";
}

ItemDesc EdgeRequestComplete::GetItemDesc() const {
  StringBuilder ts;
  ts << EdgeRequestResponse::GetItemDesc() << " [" << resource_type_ << "]";
  return ts.ReleaseString();
}

void EdgeRequestComplete::AddGraphMLAttributes(xmlDocPtr doc,
                                               xmlNodePtr parent_node) const {
  EdgeRequestResponse::AddGraphMLAttributes(doc, parent_node);
  GraphMLAttrDefForType(kGraphMLAttrDefResourceType)
      ->AddValueNode(doc, parent_node, resource_type_);
  GraphMLAttrDefForType(kGraphMLAttrDefResponseHash)
      ->AddValueNode(doc, parent_node, hash_);
}

bool EdgeRequestComplete::IsEdgeRequestComplete() const {
  return true;
}

}  // namespace brave_page_graph
