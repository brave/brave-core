/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ZCASH_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ZCASH_UTILS_H_

#include <string>
#include <vector>

namespace brave_wallet {

std::string PubkeyToTransparentAddress(const std::vector<uint8_t>& pubkey,
                                       bool testnet);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_ZCASH_UTILS_H_
