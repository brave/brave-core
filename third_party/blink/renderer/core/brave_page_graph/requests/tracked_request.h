/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_REQUESTS_TRACKED_REQUEST_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_REQUESTS_TRACKED_REQUEST_H_

#include "base/containers/span.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/utilities/response_metadata.h"
#include "third_party/blink/renderer/platform/crypto.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/wtf/vector.h"

namespace brave_page_graph {

class GraphNode;
class NodeResource;
class ResponseMetadata;

class TrackedRequest {
 public:
  // Constructor for when we see the outgoing request first.
  TrackedRequest(const InspectorId request_id,
                 GraphNode* requester,
                 NodeResource* resource,
                 const String& resource_type);
  ~TrackedRequest();

  bool IsComplete() const;

  InspectorId GetRequestId() const;
  const Vector<GraphNode*>& GetRequesters() const;
  const String& GetResourceType() const;
  NodeResource* GetResource() const;
  bool GetIsError() const;

  void AddRequest(GraphNode* requester,
                  NodeResource* resource,
                  const String& request_type);
  void SetIsError();
  void SetCompleted();

  ResponseMetadata& GetResponseMetadata();
  const ResponseMetadata& GetResponseMetadata() const;

  const String& GetResponseBodyHash() const;
  void UpdateResponseBodyHash(base::span<const char> data);

 protected:
  void FinishResponseBodyHash();

  enum class RequestStatus : uint8_t { kError = 0, kSuccess, kUnknown };

  const InspectorId request_id_;

  Vector<GraphNode*> requesters_;
  String resource_type_;

  NodeResource* resource_ = nullptr;

  RequestStatus request_status_ = RequestStatus::kUnknown;

  mutable bool is_complete_ = false;

  ResponseMetadata response_metadata_;
  int64_t size_ = -1;
  blink::Digestor body_digestor_{blink::kHashAlgorithmSha256};
  String hash_;
};

}  // namespace brave_page_graph

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_REQUESTS_TRACKED_REQUEST_H_
