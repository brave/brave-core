/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BRAVE_WALLET_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BRAVE_WALLET_CONSTANTS_H_

namespace brave_wallet {

inline constexpr int kSolanaKeypairSize = 64;
inline constexpr int kSolanaSignatureSize = 64;
inline constexpr int kSolanaPrikeySize = 32;
inline constexpr int kSolanaPubkeySize = 32;
inline constexpr int kSolanaHashSize = 32;
// 1232 = 1280(IPv6 minimum MTU) - 40(size of the IPv6 header) - 8(size of the
// fragment header)
inline constexpr size_t kSolanaMaxTxSize = 1232;

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BRAVE_WALLET_CONSTANTS_H_
