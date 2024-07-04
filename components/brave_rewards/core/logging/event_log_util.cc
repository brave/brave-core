/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/logging/event_log_util.h"
#include "brave/components/brave_rewards/core/logging/event_log_keys.h"

namespace brave_rewards::internal::log {
std::string GetEventLogKeyForLinkingResult(
    mojom::ConnectExternalWalletResult result) {
  switch (result) {
    case mojom::ConnectExternalWalletResult::kDeviceLimitReached:
      return log::kDeviceLimitReached;
    case mojom::ConnectExternalWalletResult::kFlaggedWallet:
      return log::kFlaggedWallet;
    case mojom::ConnectExternalWalletResult::kMismatchedCountries:
      return log::kMismatchedCountries;
    case mojom::ConnectExternalWalletResult::kMismatchedProviderAccounts:
      return log::kMismatchedProviderAccounts;
    case mojom::ConnectExternalWalletResult::kKYCRequired:
      return log::kKYCRequired;
    case mojom::ConnectExternalWalletResult::kProviderUnavailable:
      return log::kProviderUnavailable;
    case mojom::ConnectExternalWalletResult::kRegionNotSupported:
      return log::kRegionNotSupported;
    case mojom::ConnectExternalWalletResult::
        kRequestSignatureVerificationFailure:
      return log::kRequestSignatureVerificationFailure;
    case mojom::ConnectExternalWalletResult::
        kUpholdTransactionVerificationFailure:
      return log::kTransactionVerificationFailure;
    default:
      return "";
  }
}
}  // namespace brave_rewards::internal::log
