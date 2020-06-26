/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_REQUESTS_TRACKED_REQUEST_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_REQUESTS_TRACKED_REQUEST_H_

#include <vector>
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "brave/third_party/blink/brave_page_graph/types.h"
#include "brave/third_party/blink/brave_page_graph/utilities/response_metadata.h"

namespace brave_page_graph {

class Node;
class NodeResource;
class ResponseMetadata;

class TrackedRequest {
friend class RequestTracker;
 public:
  TrackedRequest() = delete;
  // Constructor for when we see the outgoing request first.
  TrackedRequest(const InspectorId request_id, Node* const requester,
    NodeResource* const resource, const RequestType request_type);
  // Constructor for when a successful response comes first (i.e. cached
  // replies).
  TrackedRequest(const InspectorId request_id, const blink::ResourceType type);
  // Constructor for when a failed response comes first.
  TrackedRequest(const InspectorId request_id);
  ~TrackedRequest();

  bool IsComplete() const;

  InspectorId GetRequestId() const;
  const std::vector<Node*>& GetRequesters() const;
  RequestType GetRequestType() const;
  NodeResource* GetResource() const;
  bool GetIsError() const;
  blink::ResourceType GetResourceType() const;

  void AddRequest(Node* const requester, NodeResource* const resource,
    const RequestType request_type);
  void SetIsError();
  void SetCompletedResourceType(const blink::ResourceType type);

  const ResponseMetadata& GetResponseMetadata() const;
  void SetResponseMetadata(const ResponseMetadata& metadata);

  const std::string& GetResponseBodyHash() const;
  void SetResponseBodyHash(const std::string& response_body_hash);

 protected:
  enum class RequestStatus : uint8_t {
    kError = 0,
    kSuccess,
    kUnknown
  };

  const InspectorId request_id_;

  std::vector<Node*> requesters_;
  RequestType request_type_ = RequestType::kRequestTypeUnknown;

  NodeResource* resource_ = nullptr;

  RequestStatus request_status_ = RequestStatus::kUnknown;
  blink::ResourceType resource_type_ = blink::ResourceType::kMaxValue;

  mutable bool is_complete_ = false;

  ResponseMetadata response_metadata_;
  std::string hash_;
};

}  // namsepace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_REQUESTS_TRACKED_REQUEST_H_
