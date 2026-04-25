/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_WALLET_WALLET_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_WALLET_WALLET_UTIL_H_

#include <optional>
#include <string>

#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

struct WalletInfo;

std::optional<WalletInfo> CreateWalletFromRecoverySeed(
    const std::string& payment_id,
    const std::string& recovery_seed_base64);

std::optional<WalletInfo> CreateWalletFromRecoverySeed(
    const mojom::WalletInfo* mojom_wallet);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_WALLET_WALLET_UTIL_H_
