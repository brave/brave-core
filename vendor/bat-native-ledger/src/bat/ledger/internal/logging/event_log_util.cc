/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/logging/event_log_util.h"
#include "bat/ledger/internal/logging/event_log_keys.h"

namespace ledger {
namespace log {
std::string GetEventLogKeyForLinkingResult(mojom::Result result) {
  switch (result) {
    case mojom::Result::DEVICE_LIMIT_REACHED:
      return log::kDeviceLimitReached;
    case mojom::Result::FLAGGED_WALLET:
      return log::kFlaggedWallet;
    case mojom::Result::MISMATCHED_PROVIDER_ACCOUNT_REGIONS:
      return log::kMismatchedProviderAccountRegions;
    case mojom::Result::MISMATCHED_PROVIDER_ACCOUNTS:
      return log::kMismatchedProviderAccounts;
    case mojom::Result::NOT_FOUND:
      return log::kKYCRequired;
    case mojom::Result::REGION_NOT_SUPPORTED:
      return log::kRegionNotSupported;
    case mojom::Result::REQUEST_SIGNATURE_VERIFICATION_FAILURE:
      return log::kRequestSignatureVerificationFailure;
    case mojom::Result::UPHOLD_TRANSACTION_VERIFICATION_FAILURE:
      return log::kTransactionVerificationFailure;
    default:
      NOTREACHED();
      return "";
  }
}
}  // namespace log
}  // namespace ledger
