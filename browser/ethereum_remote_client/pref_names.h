/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_ETHEREUM_REMOTE_CLIENT_PREF_NAMES_H_
#define BRAVE_BROWSER_ETHEREUM_REMOTE_CLIENT_PREF_NAMES_H_

extern const char kERCAES256GCMSivNonce[];
extern const char kERCEncryptedSeed[];
// Deprecated in favor of kBraveWalletWeb3Provider
extern const char kERCEnabledDeprecated[];
extern const char kERCPrefVersion[];
extern const char kERCLoadCryptoWalletsOnStartup[];
extern const char kERCOptedIntoCryptoWallets[];

#endif  // BRAVE_BROWSER_ETHEREUM_REMOTE_CLIENT_PREF_NAMES_H_
