/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/renderer/core/brave_page_graph/requests/request_tracker.h"

#include <memory>
#include <string>
#include <utility>

#include "base/containers/contains.h"
#include "base/memory/scoped_refptr.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/edge/request/edge_request_start.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/graph_node.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/graph_item/node/node_resource.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/requests/tracked_request.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "third_party/blink/renderer/platform/wtf/text/wtf_string.h"

namespace brave_page_graph {

RequestTracker::RequestTracker(PageGraphContext* page_graph_context)
    : page_graph_context_(page_graph_context) {
  DCHECK(page_graph_context_);
}

RequestTracker::~RequestTracker() = default;

scoped_refptr<const TrackedRequestRecord> RequestTracker::RegisterRequestStart(
    const InspectorId request_id,
    GraphNode* requester,
    const FrameId& frame_id,
    NodeResource* resource,
    const String& resource_type) {
  auto item = tracked_requests_.find(request_id);
  if (item == tracked_requests_.end()) {
    auto request_record = std::make_unique<TrackedRequest>(
        page_graph_context_, request_id, requester, frame_id, resource,
        resource_type);
    CheckTracedRequestAgainstHistory(request_record.get());
    auto tracking_record = base::MakeRefCounted<TrackedRequestRecord>();
    tracking_record->request = std::move(request_record);
    tracked_requests_.insert(request_id, tracking_record);
    return tracking_record;
  }

  item->value->request->AddRequest(requester, frame_id, resource,
                                   resource_type);
  return ReturnTrackingRecord(request_id);
}

void RequestTracker::RegisterRequestRedirect(
    const InspectorId request_id,
    const FrameId& frame_id,
    const blink::KURL& url,
    const blink::ResourceResponse& redirect_response,
    NodeResource* resource) {
  auto& request = tracked_requests_.at(request_id)->request;
  request->AddRequestRedirect(url, redirect_response, resource, frame_id);
}

scoped_refptr<const TrackedRequestRecord>
RequestTracker::RegisterRequestComplete(const InspectorId request_id,
                                        int64_t encoded_data_length,
                                        const FrameId& frame_id) {
  auto& request = tracked_requests_.at(request_id)->request;
  request->GetResponseMetadata().SetEncodedDataLength(encoded_data_length);
  request->SetCompleted(frame_id);
  return ReturnTrackingRecord(request_id);
}

scoped_refptr<const TrackedRequestRecord> RequestTracker::RegisterRequestError(
    const InspectorId request_id,
    const FrameId& frame_id) {
  tracked_requests_.at(request_id)->request->SetIsError(frame_id);
  return ReturnTrackingRecord(request_id);
}

void RequestTracker::RegisterDocumentRequestStart(
    const InspectorId request_id,
    const FrameId& frame_id,
    const blink::KURL& url,
    const bool is_main_frame,
    const base::TimeDelta timestamp) {
  // Any previous document requests from this root should have been canceled.
  if (document_request_initiators_.Contains(frame_id)) {
    CHECK_EQ(document_request_initiators_.at(frame_id), request_id);
    return;
  }

  // If we get to this point, there should be no previous request with this
  // request ID.
  CHECK(!base::Contains(document_requests_, request_id));
  DocumentRequest request_record{
      .request_id = request_id,
      .url = url,
      .is_main_frame = is_main_frame,
      .start_timestamp = timestamp,
  };
  document_request_initiators_.insert(frame_id, request_id);
  document_requests_.insert(request_id, std::move(request_record));
}

void RequestTracker::RegisterDocumentRequestComplete(
    const InspectorId request_id,
    const FrameId& frame_id,
    const int64_t encoded_data_length,
    const base::TimeDelta timestamp) {
  // The request should have been started previously.
  auto request_record_it = document_requests_.find(request_id);
  CHECK(request_record_it != document_requests_.end());
  auto& request_record = request_record_it->value;

  // The request should not have been completed previously.
  DCHECK_EQ(request_record.response_metadata.EncodedDataLength(), -1);
  DCHECK_EQ(request_record.complete_timestamp, base::TimeDelta());

  request_record.response_metadata.SetEncodedDataLength(encoded_data_length);
  request_record.complete_timestamp = timestamp;
  request_record.frame_id = frame_id;
}

DocumentRequest* RequestTracker::GetDocumentRequestInfo(
    const InspectorId request_id) {
  auto request_it = document_requests_.find(request_id);
  return request_it != document_requests_.end() ? &request_it->value : nullptr;
}

TrackedRequestRecord* RequestTracker::GetTrackingRecord(
    const InspectorId request_id) {
  auto record_it = tracked_requests_.find(request_id);
  return record_it != tracked_requests_.end() ? record_it->value.get()
                                              : nullptr;
}

scoped_refptr<const TrackedRequestRecord> RequestTracker::ReturnTrackingRecord(
    const InspectorId request_id) {
  auto record_it = tracked_requests_.find(request_id);
  auto& record = record_it->value;
  TrackedRequest* request = record->request.get();

  if (!request->IsComplete()) {
    return record_it->value;
  }

  const size_t num_requestors = request->GetRequesters().size();
  record->is_first_reply = (record->num_complete_replies == 0);
  record->num_complete_replies += 1;

  // If this is the last requester we need to reply, then
  // we want to set things up so that we loose our handle on the std::unique_ptr
  // on return.  Otherwise, we can just leave it in place.
  if (record->num_complete_replies < num_requestors) {
    return record;
  }

  AddTracedRequestToHistory(request);
  auto record_shared = std::move(record);
  tracked_requests_.erase(record_it);
  return record_shared;
}

void RequestTracker::AddTracedRequestToHistory(const TrackedRequest* request) {
  CHECK_GT(request->GetRequestId(), 0ull);
  CHECK(request->GetResource());
  completed_requests_.insert(request->GetRequestId(), request->GetResource());
}

void RequestTracker::CheckTracedRequestAgainstHistory(
    const TrackedRequest* request) {
  const auto request_it = completed_requests_.find(request->GetRequestId());
  CHECK(request_it == completed_requests_.end() ||
        request_it->value == request->GetResource());
}

}  // namespace brave_page_graph
