/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "base/containers/contains.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"
#include "src/components/permissions/permission_request_manager.cc"
#include "url/origin.h"

namespace permissions {

bool PermissionRequestManager::ShouldGroupRequests(PermissionRequest* a,
                                                   PermissionRequest* b) {
  url::Origin origin_a;
  url::Origin origin_b;
  if (a->request_type() == RequestType::kBraveEthereum ||
      a->request_type() == RequestType::kBraveSolana) {
    if (a->request_type() == b->request_type() &&
        brave_wallet::ParseRequestingOriginFromSubRequest(
            a->request_type(), url::Origin::Create(a->requesting_origin()),
            &origin_a, nullptr) &&
        brave_wallet::ParseRequestingOriginFromSubRequest(
            b->request_type(), url::Origin::Create(b->requesting_origin()),
            &origin_b, nullptr) &&
        origin_a == origin_b) {
      return true;
    }
  }

  return ::permissions::ShouldGroupRequests(a, b);
}

// Accept/Deny/Cancel each sub-request, total size of all passed in requests
// should be equal to current requests_size because we will finalizing all
// current requests in the end. The request callbacks will be called in FIFO
// order.
void PermissionRequestManager::AcceptDenyCancel(
    const std::vector<PermissionRequest*>& accepted_requests,
    const std::vector<PermissionRequest*>& denied_requests,
    const std::vector<PermissionRequest*>& cancelled_requests) {
  if (ignore_callbacks_from_prompt_)
    return;

  DCHECK(view_);
  DCHECK((accepted_requests.size() + denied_requests.size() +
          cancelled_requests.size()) == requests_.size());

  // We need to process requests in reverse order because
  // PermissionRequestQueue impelementation of Push and Pop are paired with
  // base::circular_dequeue's (push_back, pop_back) and (push_front, pop_front)
  // Once pending_permission_requests_ is popped to the vector request_, the
  // order is fixed because the reorder takes place in
  // pending_permission_requests_.
  for (auto it = requests_.crbegin(); it != requests_.crend(); ++it) {
    if (base::Contains(accepted_requests, *it)) {
      PermissionGrantedIncludingDuplicates(*it, /*is_one_time=*/false);
    } else if (base::Contains(denied_requests, *it)) {
      PermissionDeniedIncludingDuplicates(*it);
    } else {
      CancelledIncludingDuplicates(*it);
    }
  }

  // Finalize permission with granted if some sub-requests are accepted. If
  // no requests are accepted, finalize with denied if some sub-requests are
  // denied. Otherwise, finalize with dismissed.
  // TODO(jocelyn): This does not have any bad impact atm if we finalize all
  // requests with GRANTED option in the situation that some sub-requests are
  // not granted, but we need to take a deeper look to see how we can finalize
  // requests with different actions.
  PermissionAction action = PermissionAction::DISMISSED;
  if (!accepted_requests.empty()) {
    action = PermissionAction::GRANTED;
  } else if (!denied_requests.empty()) {
    action = PermissionAction::DENIED;
  }
  CurrentRequestsDecided(action);
}

}  // namespace permissions
