/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <optional>
#include <string>

#include "base/check.h"
#include "base/check_is_test.h"
#include "base/containers/contains.h"
#include "brave/components/brave_wallet/browser/permission_utils.h"

#define BRAVE_PERMISSION_REQUEST_MANAGER_GET_REQUESTING_ORIGIN \
  if (!ShouldBeGrouppedInRequests(request.get()))

// |tab_is_hidden_| should be updated after upstream sets.
#define BRAVE_PERMISSION_REQUEST_MANAGER_ON_VISIBILITY_CHANGED \
  UpdateTabIsHiddenWithTabActivationState();

#include "src/components/permissions/permission_request_manager.cc"

#undef BRAVE_PERMISSION_REQUEST_MANAGER_ON_VISIBILITY_CHANGED
#undef BRAVE_PERMISSION_REQUEST_MANAGER_GET_REQUESTING_ORIGIN

#include "url/origin.h"

namespace permissions {

bool PermissionRequestManager::ShouldGroupRequests(PermissionRequest* a,
                                                   PermissionRequest* b) const {
  url::Origin origin_a;
  url::Origin origin_b;
  if (a->request_type() == RequestType::kBraveEthereum ||
      a->request_type() == RequestType::kBraveSolana ||
      a->request_type() == RequestType::kBraveCardano) {
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

bool PermissionRequestManager::ShouldBeGrouppedInRequests(
    PermissionRequest* a) const {
  DCHECK(!requests_.empty());
  // Called from PermissionRequestManager::GetRequestingOrigin when DCHECK IS ON
  // to adjust the check for grouped requests. |requests_| is cheked by the
  // caller to not be empty.
  if (requests_.front().get() == a) {
    return true;
  }
  return ShouldGroupRequests(requests_.front().get(), a);
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

  for (const auto& request : requests_) {
    if (base::Contains(accepted_requests, request.get())) {
      PermissionGrantedIncludingDuplicates(request.get(),
                                           /*is_one_time=*/false);
    } else if (base::Contains(denied_requests, request.get())) {
      PermissionDeniedIncludingDuplicates(request.get());
    } else {
      CancelRequestIncludingDuplicates(request.get());
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

void PermissionRequestManager::OnTabActiveStateChanged(bool active) {
  tab_is_activated_ = active;

  // OnVisibilityChanged() has logic for |tab_is_hidden_| state changes.
  // Tab activation state could affect |tab_is_hidden_| state.
  OnVisibilityChanged(web_contents()->GetVisibility());
}

void PermissionRequestManager::UpdateTabIsHiddenWithTabActivationState() {
  if (!tab_is_activated_.has_value()) {
    return;
  }

  // In split view, permission manager can have invalid tab hidden state.
  // If it's inactive split tab, permission manager should set false
  // to |tab_is_hidden_| to prevent launching permission bubble from
  // that inactive split tab. Otherwise, it launches permission bubble even
  // it's inactive tab.
  if (!tab_is_hidden_ && !tab_is_activated_.value()) {
    tab_is_hidden_ = true;
  }
}

}  // namespace permissions
