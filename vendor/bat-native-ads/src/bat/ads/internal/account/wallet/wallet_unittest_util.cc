/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/wallet/wallet_unittest_util.h"

#include <vector>

#include "absl/types/optional.h"
#include "base/base64.h"
#include "base/check.h"
#include "bat/ads/internal/account/wallet/wallet.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"

namespace ads {

namespace {

constexpr char kPaymentId[] = "27a39b2f-9b2e-4eb0-bbb2-2f84447496e7";
constexpr char kValidRecoverySeed[] =
    "x5uBvgI5MTTVY6sjGv65e9EHr8v7i+UxkFB9qVc5fP0=";
constexpr char kInvalidRecoverySeed[] =
    "y6vCwhJ6NUUWZ7tkHw76f0FIs9w8j-VylGC0rWd6gQ1=";

}  // namespace

std::string GetWalletPaymentIdForTesting() {
  return kPaymentId;
}

std::string GetWalletRecoverySeedForTesting() {
  return kValidRecoverySeed;
}

std::string GetInvalidWalletRecoverySeedForTesting() {
  return kInvalidRecoverySeed;
}

WalletInfo GetWalletForTesting() {
  const absl::optional<std::vector<uint8_t>> raw_recovery_seed =
      base::Base64Decode(kValidRecoverySeed);
  CHECK(raw_recovery_seed);

  Wallet wallet;
  wallet.Set(kPaymentId, *raw_recovery_seed);
  return wallet.Get();
}

}  // namespace ads
