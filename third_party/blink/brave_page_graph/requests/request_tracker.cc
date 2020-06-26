/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/third_party/blink/brave_page_graph/requests/request_tracker.h"
#include <map>
#include <memory>
#include <string>
#include <utility>
#include "third_party/blink/renderer/platform/loader/fetch/resource.h"
#include "brave/third_party/blink/brave_page_graph/logging.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/edge/request/edge_request_start.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node_resource.h"
#include "brave/third_party/blink/brave_page_graph/graph_item/node/node.h"
#include "brave/third_party/blink/brave_page_graph/requests/tracked_request.h"
#include "brave/third_party/blink/brave_page_graph/types.h"

using ::std::make_pair;
using ::std::make_shared;
using ::std::make_unique;
using ::std::move;
using ::std::shared_ptr;
using ::std::unique_ptr;

namespace brave_page_graph {

RequestTracker::RequestTracker() {}

RequestTracker::~RequestTracker() {}

shared_ptr<const TrackedRequestRecord> RequestTracker::RegisterRequestStart(
    const InspectorId request_id, Node* const requester,
    NodeResource* const resource, const RequestType request_type) {

  if (tracked_requests_.count(request_id) == 0) {
    auto request_record = make_unique<TrackedRequest>(request_id, requester,
      resource, request_type);
    CheckTracedRequestAgainstHistory(request_record.get());
    auto tracking_record = make_shared<TrackedRequestRecord>();
    tracking_record->request = move(request_record);
    tracked_requests_.emplace(request_id, tracking_record);
    return tracking_record;
  }

  tracked_requests_.at(request_id)->request->AddRequest(requester, resource,
    request_type);
  return ReturnTrackingRecord(request_id);
}

shared_ptr<const TrackedRequestRecord> RequestTracker::RegisterRequestComplete(
    const InspectorId request_id, const blink::ResourceType type) {

  if (tracked_requests_.count(request_id) == 0) {
    auto request_record = make_unique<TrackedRequest>(request_id, type);
    auto tracking_record = make_shared<TrackedRequestRecord>();
    tracking_record->request = move(request_record);
    tracked_requests_.emplace(request_id, tracking_record);
    return tracking_record;
  }

  tracked_requests_.at(request_id)->request->SetCompletedResourceType(type);
  return ReturnTrackingRecord(request_id);
}

shared_ptr<const TrackedRequestRecord> RequestTracker::RegisterRequestError(
    const InspectorId request_id) {

  if (tracked_requests_.count(request_id) == 0) {
    auto request_record = make_unique<TrackedRequest>(request_id);
    auto tracking_record = make_shared<TrackedRequestRecord>();
    tracking_record->request = move(request_record);
    tracked_requests_.emplace(request_id, tracking_record);
    return tracking_record;
  }

  tracked_requests_.at(request_id)->request->SetIsError();
  return ReturnTrackingRecord(request_id);
}

shared_ptr<const TrackedRequestRecord> RequestTracker::ReturnTrackingRecord(
    const InspectorId request_id) {

  TrackedRequestRecord* record = tracked_requests_.at(request_id).get();
  TrackedRequest* request = record->request.get();

  if (request->IsComplete() == false) {
    return tracked_requests_.at(request_id);
  }

  const size_t num_requestors = request->GetRequesters().size();
  record->is_first_reply = (record->num_complete_replies == 0);
  record->num_complete_replies += 1;

  // If this is the last requester we need to reply, then
  // we want to set things up so that we loose our handle on the unique_ptr
  // on return.  Otherwise, we can just leave it in place.
  if (record->num_complete_replies < num_requestors) {
    return tracked_requests_.at(request_id);
  }

  AddTracedRequestToHistory(request);
  shared_ptr<const TrackedRequestRecord> record_shared =
    tracked_requests_.at(request_id);
  tracked_requests_.erase(request_id);
  return record_shared;
}

void RequestTracker::AddTracedRequestToHistory(const TrackedRequest* request) {
  LOG_ASSERT(request->GetRequestId() > 0);
  LOG_ASSERT(request->GetResource() != nullptr);
  completed_requests_.emplace(request->GetRequestId(), request->GetResource());
}

void RequestTracker::CheckTracedRequestAgainstHistory(
    const TrackedRequest* request) {
  LOG_ASSERT(completed_requests_.count(request->GetRequestId()) == 0 ||
    completed_requests_.at(request->GetRequestId()) == request->GetResource());
}

}  // namsepace brave_page_graph
