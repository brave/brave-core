/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_REQUESTS_REQUEST_TRACKER_H_
#define BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_REQUESTS_REQUEST_TRACKER_H_

#include <memory>
#include <string>
#include <utility>

#include "base/memory/ref_counted.h"
#include "base/time/time.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/page_graph_context.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/types.h"
#include "brave/third_party/blink/renderer/core/brave_page_graph/utilities/response_metadata.h"
#include "third_party/blink/renderer/platform/loader/fetch/resource_response.h"
#include "third_party/blink/renderer/platform/weborigin/kurl.h"
#include "third_party/blink/renderer/platform/wtf/hash_map.h"

namespace brave_page_graph {

class NodeResource;
class GraphNode;
class TrackedRequest;

struct TrackedRequestRecord : public base::RefCounted<TrackedRequestRecord> {
  std::unique_ptr<TrackedRequest> request;
  uint16_t num_complete_replies = 0;
  bool is_first_reply;
};

struct DocumentRequest {
  // Information available at request start
  InspectorId request_id;
  FrameId frame_id;
  blink::KURL url;
  bool is_main_frame;
  base::TimeDelta start_timestamp;

  // Information available at response
  ResponseMetadata response_metadata;

  // Information available at request completion
  base::TimeDelta complete_timestamp;
};

class RequestTracker {
 public:
  explicit RequestTracker(PageGraphContext* page_graph_context);
  ~RequestTracker();

  scoped_refptr<const TrackedRequestRecord> RegisterRequestStart(
      const InspectorId request_id,
      GraphNode* requester,
      const FrameId& frame_id,
      NodeResource* resource,
      const String& resource_type);
  void RegisterRequestRedirect(const InspectorId request_id,
                               const FrameId& frame_id,
                               const blink::KURL& url,
                               const blink::ResourceResponse& redirect_response,
                               NodeResource* resource);
  scoped_refptr<const TrackedRequestRecord> RegisterRequestComplete(
      const InspectorId request_id,
      int64_t encoded_data_length,
      const FrameId& frame_id);
  scoped_refptr<const TrackedRequestRecord> RegisterRequestError(
      const InspectorId request_id,
      const FrameId& frame_id);

  void RegisterDocumentRequestStart(const InspectorId request_id,
                                    const FrameId& frame_id,
                                    const blink::KURL& url,
                                    const bool is_main_frame,
                                    const base::TimeDelta timestamp);
  void RegisterDocumentRequestComplete(const InspectorId request_id,
                                       const FrameId& frame_id,
                                       const int64_t encoded_data_length,
                                       const base::TimeDelta timestamp);
  DocumentRequest* GetDocumentRequestInfo(const InspectorId request_id);
  TrackedRequestRecord* GetTrackingRecord(const InspectorId request_id);

 private:
  HashMap<InspectorId, scoped_refptr<TrackedRequestRecord>> tracked_requests_;

  HashMap<FrameId, InspectorId> document_request_initiators_;
  HashMap<InspectorId, DocumentRequest> document_requests_;

  // Returns the record from the above map, and cleans up the record
  // if the final requester has been responded to.
  scoped_refptr<const TrackedRequestRecord> ReturnTrackingRecord(
      const InspectorId request_id);

  PageGraphContext* const page_graph_context_ = nullptr;

  // This structure is just included for debugging, to make sure the
  // assumptions built into this request tracking system (e.g. that
  // request ids will not repeat, etc.).
  HashMap<InspectorId, const NodeResource*> completed_requests_;

  // These methods manage writing to and from the above structure.
  void AddTracedRequestToHistory(const TrackedRequest* request);
  // Checks to make sure that either 1) this request id hasn't been seen
  // before, or 2) that if its been seen before, its to the same Resource.
  void CheckTracedRequestAgainstHistory(const TrackedRequest* request);
};

}  // namespace brave_page_graph

#endif  // BRAVE_THIRD_PARTY_BLINK_RENDERER_CORE_BRAVE_PAGE_GRAPH_REQUESTS_REQUEST_TRACKER_H_
