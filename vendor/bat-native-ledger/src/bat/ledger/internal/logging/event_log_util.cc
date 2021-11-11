/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/logging/event_log_util.h"
#include "bat/ledger/internal/logging/event_log_keys.h"

namespace ledger {
namespace log {
std::string GetEventLogKeyForLinkingResult(type::Result result) {
  switch (result) {
    case type::Result::DEVICE_LIMIT_REACHED:
      return log::kDeviceLimitReached;
    case type::Result::MISMATCHED_PROVIDER_ACCOUNTS:
      return log::kMismatchedProviderAccounts;
    case type::Result::NOT_FOUND:
      return log::kKYCRequired;
    case type::Result::REQUEST_SIGNATURE_VERIFICATION_FAILURE:
      return log::kRequestSignatureVerificationFailure;
    case type::Result::UPHOLD_TRANSACTION_VERIFICATION_FAILURE:
      return log::kTransactionVerificationFailure;
    default:
      NOTREACHED();
      return "";
  }
}
}  // namespace log
}  // namespace ledger
