/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/requests/tracked_request.h"

#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/request/edge_request_complete.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/request/edge_request_error.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/request/edge_request_redirect.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/request/edge_request_start.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/graph_node.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/node_resource.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/page_graph_context.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/utilities/response_metadata.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/wtf/text/base64.h"

namespace brave_page_graph {

TrackedRequest::~TrackedRequest() = default;

// Constructor for when we see the outgoing request first.
TrackedRequest::TrackedRequest(PageGraphContext* page_graph_context,
                               const InspectorId request_id,
                               GraphNode* requester,
                               NodeResource* resource,
                               const String& resource_type)
    : page_graph_context_(page_graph_context),
      request_id_(request_id),
      resource_type_(resource_type) {
  DCHECK(page_graph_context_);
  DCHECK(requester);
  DCHECK(resource);
  requesters_.push_back(requester);
  resource_ = resource;

  page_graph_context_->AddEdge<EdgeRequestStart>(requester, resource,
                                                 request_id, resource_type);
}

bool TrackedRequest::IsComplete() const {
  if (is_complete_) {
    return true;
  }

  if (requesters_.empty() || !resource_ || !request_status_) {
    return false;
  }

  is_complete_ = true;
  return true;
}

InspectorId TrackedRequest::GetRequestId() const {
  return request_id_;
}

const Vector<GraphNode*>& TrackedRequest::GetRequesters() const {
  return requesters_;
}

NodeResource* TrackedRequest::GetResource() const {
  return resource_;
}

bool TrackedRequest::GetIsError() const {
  return request_status_ == RequestStatus::kError;
}

const String& TrackedRequest::GetResourceType() const {
  return resource_type_;
}

void TrackedRequest::AddRequest(GraphNode* requester,
                                NodeResource* resource,
                                const String& resource_type) {
  CHECK(requester != nullptr);
  CHECK(resource != nullptr);
  CHECK(!resource_type.empty());

  if (requesters_.size() != 0) {
    // These assertions check that we're only seeing the same
    // resource id / InspectorID reused when making identical requests
    // to the identical resource. If this is wrong, then my understanding
    // of the blink request system is wrong...
    CHECK(resource_type == resource_type_);
    CHECK(resource == resource_);
  } else {
    resource_type_ = resource_type;
    resource_ = resource;
  }

  requesters_.push_back(requester);
}

void TrackedRequest::AddRequestRedirect(
    const blink::KURL& url,
    const blink::ResourceResponse& redirect_response,
    NodeResource* resource) {
  ResponseMetadata metadata;
  metadata.ProcessResourceResponse(redirect_response);
  page_graph_context_->AddEdge<EdgeRequestRedirect>(resource_, resource,
                                                    request_id_, metadata);

  requesters_.push_back(resource_);
  resource_ = resource;
}

void TrackedRequest::SetIsError() {
  // Check that we haven't tried to set error information after we've
  // already set information about a successful response.
  CHECK(!request_status_ || request_status_ == RequestStatus::kError);
  const bool status_was_empty = !request_status_;
  request_status_ = RequestStatus::kError;
  FinishResponseBodyHash();
  if (status_was_empty) {
    page_graph_context_->AddEdge<EdgeRequestError>(
        resource_, requesters_.front(), request_id_, GetResponseMetadata());
  }
}

void TrackedRequest::SetCompleted() {
  // Check that we haven't tried to set "successful response" information
  // after we've already set information about an error.
  CHECK(!request_status_ || request_status_ == RequestStatus::kSuccess);
  const bool status_was_empty = !request_status_;
  request_status_ = RequestStatus::kSuccess;
  FinishResponseBodyHash();
  if (status_was_empty) {
    page_graph_context_->AddEdge<EdgeRequestComplete>(
        resource_, requesters_.front(), request_id_, resource_type_,
        GetResponseMetadata(), GetResponseBodyHash());
  }
}

ResponseMetadata& TrackedRequest::GetResponseMetadata() {
  return response_metadata_;
}

const ResponseMetadata& TrackedRequest::GetResponseMetadata() const {
  return response_metadata_;
}

const String& TrackedRequest::GetResponseBodyHash() const {
  CHECK(request_status_ == RequestStatus::kSuccess);
  CHECK(!hash_.empty());
  return hash_;
}

void TrackedRequest::UpdateResponseBodyHash(base::span<const char> data) {
  CHECK(request_status_ != RequestStatus::kSuccess);
  if (!data.data()) {
    return;
  }
  base::span<const uint8_t> uint8_span(
      reinterpret_cast<const uint8_t*>(data.data()), data.size());
  CHECK(body_digestor_.Update(uint8_span));
}

void TrackedRequest::FinishResponseBodyHash() {
  CHECK(hash_.empty());
  blink::DigestValue digest;
  CHECK(body_digestor_.Finish(digest));
  hash_ = WTF::Base64Encode(digest);
}

}  // namespace brave_page_graph
