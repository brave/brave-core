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

namespace {

bool IsAccepted(PermissionRequest* request,
                const std::vector<std::string>& accounts) {
  for (auto account : accounts) {
    if (base::EndsWith(request->GetOrigin().host(), account,
                       base::CompareCase::INSENSITIVE_ASCII)) {
      return true;
    }
  }

  return false;
}

}  // namespace

bool PermissionRequestManager::ShouldGroupRequests(PermissionRequest* a,
                                                   PermissionRequest* b) {
  std::string origin_a;
  std::string origin_b;
  if (a->GetRequestType() == RequestType::kBraveEthereum &&
      b->GetRequestType() == RequestType::kBraveEthereum &&
      brave_wallet::ParseRequestingOrigin(a->GetOrigin(), true, &origin_a,
                                          nullptr) &&
      brave_wallet::ParseRequestingOrigin(b->GetOrigin(), true, &origin_b,
                                          nullptr) &&
      origin_a == origin_b) {
    return true;
  }

  return ::permissions::ShouldGroupRequests(a, b);
}

void PermissionRequestManager::AcceptEthereumPermissionRequests(
    const std::vector<std::string>& accounts) {
  std::vector<PermissionRequest*>::iterator requests_iter;
  for (requests_iter = requests_.begin(); requests_iter != requests_.end();
       requests_iter++) {
    DCHECK((*requests_iter)->GetRequestType() == RequestType::kBraveEthereum);
    if (IsAccepted(*requests_iter, accounts)) {
      PermissionGrantedIncludingDuplicates(*requests_iter,
                                           /*is_one_time=*/false);
    } else {
      CancelledIncludingDuplicates(*requests_iter);
    }
  }

  // TODO(jocelyn): This does not have any bad impact atm if we finalize all
  // requests with GRANTED option in the situation that some sub-requests are
  // not granted, but we need to take a deeper look to see how we can finalize
  // requests with different actions.
  FinalizeCurrentRequests(PermissionAction::GRANTED);
}

void PermissionRequestManager::IgnoreEthereumPermissionRequests() {
  std::vector<PermissionRequest*>::iterator requests_iter;
  for (requests_iter = requests_.begin(); requests_iter != requests_.end();
       requests_iter++) {
    CancelledIncludingDuplicates(*requests_iter);
  }
  FinalizeCurrentRequests(PermissionAction::DISMISSED);
}

}  // namespace permissions
#endif  // BUILDFLAG(BRAVE_WALLET_ENABLED)
