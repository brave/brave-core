/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ETHEREUM_REMOTE_CLIENT_PREF_NAMES_H_
#define BRAVE_BROWSER_ETHEREUM_REMOTE_CLIENT_PREF_NAMES_H_

inline constexpr char kERCAES256GCMSivNonce[] =
    "brave.wallet.aes_256_gcm_siv_nonce";
inline constexpr char kERCEncryptedSeed[] = "brave.wallet.encrypted_seed";
inline constexpr char kERCPrefVersion[] = "brave.wallet.pref_version";
inline constexpr char kERCOptedIntoCryptoWallets[] = "brave.wallet.opted_in";

#endif  // BRAVE_BROWSER_ETHEREUM_REMOTE_CLIENT_PREF_NAMES_H_
