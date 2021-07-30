/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "brave/components/brave_wallet/common/buildflags/buildflags.h"

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
#include "brave/components/brave_wallet/browser/ethereum_permission_utils.h"
#endif  // BUILDFLAG(BRAVE_WALLET_ENABLED)

#include "../../../../components/permissions/permission_request_manager.cc"

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
namespace permissions {

bool PermissionRequestManager::ShouldGroupRequests(PermissionRequest* a,
                                                   PermissionRequest* b) {
  std::string origin_a;
  std::string origin_b;
  if (a->GetRequestType() == RequestType::kBraveEthereum &&
      b->GetRequestType() == RequestType::kBraveEthereum &&
      brave_wallet::ParseRequestingOriginFromSubRequest(a->GetOrigin(),
                                                        &origin_a, nullptr) &&
      brave_wallet::ParseRequestingOriginFromSubRequest(b->GetOrigin(),
                                                        &origin_b, nullptr) &&
      origin_a == origin_b) {
    return true;
  }

  return ::permissions::ShouldGroupRequests(a, b);
}

// Accept/Deny/Cancel each sub-request, total size of all passed in requests
// should be equal to current requests_size because we will finalizing all
// current requests in the end.
void PermissionRequestManager::AcceptDenyCancel(
    const std::vector<PermissionRequest*>& accepted_requests,
    const std::vector<PermissionRequest*>& denied_requests,
    const std::vector<PermissionRequest*>& cancelled_requests) {
  if (ignore_callbacks_from_prompt_)
    return;

  DCHECK(view_);
  DCHECK((accepted_requests.size() + denied_requests.size() +
          cancelled_requests.size()) == requests_.size());

  for (PermissionRequest* request : accepted_requests) {
    PermissionGrantedIncludingDuplicates(request,
                                         /*is_one_time=*/false);
  }

  for (PermissionRequest* request : denied_requests) {
    PermissionDeniedIncludingDuplicates(request);
  }

  for (PermissionRequest* request : cancelled_requests) {
    CancelledIncludingDuplicates(request);
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
  FinalizeCurrentRequests(action);
}

}  // namespace permissions
#endif  // BUILDFLAG(BRAVE_WALLET_ENABLED)
