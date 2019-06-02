/* Copyright (c) 2019 The Brave Software Team. Distributed under the MPL2
 * license. This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_REQUESTS_REQUEST_TRACKER_H_
#define BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_REQUESTS_REQUEST_TRACKER_H_

#include <map>
#include <memory>
#include <string>
#include <utility>
#include "brave/third_party/blink/brave_page_graph/types.h"

namespace brave_page_graph {

class NodeResource;
class Node;
class TrackedRequest;

typedef struct TrackedRequestRecord {
  std::unique_ptr<TrackedRequest> request;
  uint16_t num_complete_replies = 0;
  bool is_first_reply;
} TrackedRequestRecord;

class RequestTracker {
 public:
  typedef std::pair<InspectorId, const NodeResource*> RequestKey;

  RequestTracker();
  ~RequestTracker();

  std::shared_ptr<const TrackedRequestRecord> RegisterRequestStart(
    const InspectorId request_id, Node* const requester,
    NodeResource* const resource, const RequestType request_type);
  std::shared_ptr<const TrackedRequestRecord> RegisterRequestComplete(
    const InspectorId request_id, const blink::ResourceType type);
  std::shared_ptr<const TrackedRequestRecord> RegisterRequestError(
    const InspectorId request_id);

 private:
  std::map<InspectorId, std::shared_ptr<TrackedRequestRecord> > tracked_requests_;

  // Returns the record from the above map, and cleans up the record
  // if the final requester has been responded to.
  std::shared_ptr<const TrackedRequestRecord> ReturnTrackingRecord(
    const InspectorId request_id);

  // This structure is just included for debugging, to make sure the
  // assumptions built into this request tracking system (e.g. that
  // request ids will not repeat, etc.).
  std::map<InspectorId, const NodeResource*> completed_requests_;

  // These methods manage writing to and from the above structure.
  void AddTracedRequestToHistory(const TrackedRequest* request);
  // Checks to make sure that either 1) this request id hasn't been seen
  // before, or 2) that if its been seen before, its to the same Resource.
  void CheckTracedRequestAgainstHistory(const TrackedRequest* request);
};

}  // namespace brave_page_graph

#endif  // BRAVE_COMPONENTS_BRAVE_PAGE_GRAPH_REQUESTS_REQUEST_TRACKER_H_
