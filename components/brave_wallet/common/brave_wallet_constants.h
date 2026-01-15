/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BRAVE_WALLET_CONSTANTS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BRAVE_WALLET_CONSTANTS_H_

namespace brave_wallet {

inline constexpr size_t kSolanaKeypairSize = 64;
inline constexpr size_t kSolanaSignatureSize = 64;
inline constexpr size_t kSolanaPrikeySize = 32;
inline constexpr size_t kSolanaPubkeySize = 32;
inline constexpr size_t kSolanaHashSize = 32;
// 1232 = 1280(IPv6 minimum MTU) - 40(size of the IPv6 header) - 8(size of the
// fragment header)
inline constexpr size_t kSolanaMaxTxSize = 1232;

// https://docs.rs/schnorrkel/0.11.4/schnorrkel/keys/index.html#constants
inline constexpr size_t kSr25519SeedSize = 32;
inline constexpr size_t kSr25519SecretKeySize = 64;
inline constexpr size_t kSr25519PublicKeySize = 32;
inline constexpr size_t kSr25519SignatureSize = 64;
inline constexpr size_t kSr25519Pkcs8Size = 117;

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_BRAVE_WALLET_CONSTANTS_H_
