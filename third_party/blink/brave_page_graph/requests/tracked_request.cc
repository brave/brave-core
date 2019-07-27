/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/requests/tracked_request.h"
#include <vector>
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "brave/third_party/blink/brave_page_graph/logging.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_resource.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"

using ::std::vector;

namespace brave_page_graph {

TrackedRequest::~TrackedRequest() {}

// Constructor for when we see the outgoing request first.
TrackedRequest::TrackedRequest(const InspectorId request_id,
    Node* const requester, NodeResource* const resource,
    const RequestType request_type) :
      request_id_(request_id),
      request_type_(request_type),
      resource_(resource) {
  LOG_ASSERT(requester != nullptr);
  LOG_ASSERT(resource != nullptr);
  requesters_.push_back(requester);
}

// Constructor for when a successful response comes first (i.e. cached
// replies).
TrackedRequest::TrackedRequest(const InspectorId request_id,
    const blink::ResourceType type) :
      request_id_(request_id),
      resource_type_(type) {
  LOG_ASSERT(resource_type_ != blink::ResourceType::kMaxValue);
  request_status_ = RequestStatus::kSuccess;
}

// Constructor for when a failed response comes first.
TrackedRequest::TrackedRequest(const InspectorId request_id) :
      request_id_(request_id) {
  request_status_ = RequestStatus::kError;
}

bool TrackedRequest::IsComplete() const {
  if (is_complete_) {
    return true;
  }

  if (requesters_.size() == 0 ||
      request_type_ == RequestType::kRequestTypeUnknown ||
      resource_ == nullptr ||
      request_status_ == RequestStatus::kUnknown) {
    return false;
  }

  is_complete_ = true;
  return true;
}

InspectorId TrackedRequest::GetRequestId() const {
  return request_id_;
}

const vector<Node*>& TrackedRequest::GetRequesters() const {
  return requesters_;
}

RequestType TrackedRequest::GetRequestType() const {
  return request_type_;
}

NodeResource* TrackedRequest::GetResource() const {
  return resource_;
}

bool TrackedRequest::GetIsError() const {
  return request_status_ == RequestStatus::kError;
}

blink::ResourceType TrackedRequest::GetResourceType() const {
  return resource_type_;
}

void TrackedRequest::AddRequest(Node* const requester,
    NodeResource* const resource, const RequestType request_type) {
  LOG_ASSERT(requester != nullptr);
  LOG_ASSERT(resource != nullptr);
  LOG_ASSERT(request_type != RequestType::kRequestTypeUnknown);

  if (requesters_.size() != 0) {
    // These assertions check that we're only seeing the same
    // resource id / InspectorID reused when making identical requests
    // to the identical resource. If this is wrong, then my understanding
    // of the blink request system is wrong...
    LOG_ASSERT(request_type == request_type_);
    LOG_ASSERT(resource == resource_);
  } else {
    request_type_ = request_type;
    resource_ = resource;
  }

  requesters_.push_back(requester);
}

void TrackedRequest::SetIsError() {
  // Check that we haven't tried to set error information after we've
  // already set information about a successful response.
  LOG_ASSERT(request_status_ == RequestStatus::kUnknown ||
      request_status_ == RequestStatus::kError);
  request_status_ = RequestStatus::kError;
}

void TrackedRequest::SetCompletedResourceType(const blink::ResourceType type) {
  LOG_ASSERT(type != blink::ResourceType::kMaxValue);

  // Check that we haven't tried to set "successful response" information
  // after we've already set information about an error.
  LOG_ASSERT(request_status_ == RequestStatus::kUnknown ||
      request_status_ == RequestStatus::kSuccess);
  request_status_ = RequestStatus::kSuccess;
  resource_type_ = type;
}


const std::string& TrackedRequest::ResponseHeaderString() const {
  return response_header_string_;
}

void TrackedRequest::SetResponseHeaderString(
    const std::string& response_header_string) {
  response_header_string_ = response_header_string;
}

int64_t TrackedRequest::ResponseBodyLength() const {
  return response_body_length_;
}

void TrackedRequest::SetResponseBodyLength(
    const int64_t response_body_length) {
  response_body_length_ = response_body_length;
}


}  // namsepace brave_page_graph
