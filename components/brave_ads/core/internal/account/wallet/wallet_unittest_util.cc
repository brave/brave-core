/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_util.h"

#include <vector>

#include "base/base64.h"
#include "base/check.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_unittest_constants.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

WalletInfo GetWalletForTesting() {
  const absl::optional<std::vector<uint8_t>> raw_recovery_seed =
      base::Base64Decode(kWalletRecoverySeed);
  CHECK(raw_recovery_seed);

  Wallet wallet;
  wallet.Set(kWalletPaymentId, *raw_recovery_seed);
  return wallet.Get();
}

}  // namespace brave_ads
