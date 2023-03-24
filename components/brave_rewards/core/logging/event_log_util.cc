/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/logging/event_log_util.h"
#include "brave/components/brave_rewards/core/logging/event_log_keys.h"

namespace brave_rewards::core {
namespace log {
std::string GetEventLogKeyForLinkingResult(
    mojom::ConnectExternalWalletError error) {
  switch (error) {
    case mojom::ConnectExternalWalletError::kDeviceLimitReached:
      return log::kDeviceLimitReached;
    case mojom::ConnectExternalWalletError::kFlaggedWallet:
      return log::kFlaggedWallet;
    case mojom::ConnectExternalWalletError::kMismatchedCountries:
      return log::kMismatchedCountries;
    case mojom::ConnectExternalWalletError::kMismatchedProviderAccounts:
      return log::kMismatchedProviderAccounts;
    case mojom::ConnectExternalWalletError::kKYCRequired:
      return log::kKYCRequired;
    case mojom::ConnectExternalWalletError::kProviderUnavailable:
      return log::kProviderUnavailable;
    case mojom::ConnectExternalWalletError::kRegionNotSupported:
      return log::kRegionNotSupported;
    case mojom::ConnectExternalWalletError::
        kRequestSignatureVerificationFailure:
      return log::kRequestSignatureVerificationFailure;
    case mojom::ConnectExternalWalletError::
        kUpholdTransactionVerificationFailure:
      return log::kTransactionVerificationFailure;
    default:
      return "";
  }
}
}  // namespace log
}  // namespace brave_rewards::core
