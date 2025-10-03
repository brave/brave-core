/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_POLKADOT_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_POLKADOT_UTILS_H_

namespace brave_wallet {

// See definition for "path": ["sp_core", "crypto", "AccountId32"]
// https://raw.githubusercontent.com/polkadot-js/api/refs/heads/master/packages/types-support/src/metadata/v16/substrate-types.json
inline constexpr const size_t kPolkadotSubstrateAccountIdSize = 32;

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_POLKADOT_UTILS_H_
