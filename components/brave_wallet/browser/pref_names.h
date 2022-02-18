/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PREF_NAMES_H_

extern const char kDefaultWallet2[];
extern const char kDefaultBaseCurrency[];
extern const char kDefaultBaseCryptocurrency[];
extern const char kBraveWalletTransactions[];
extern const char kShowWalletIconOnToolbar[];
extern const char kBraveWalletLastUnlockTime[];
extern const char kBraveWalletKeyrings[];
extern const char kBraveWalletCustomNetworks[];
extern const char kBraveWalletCurrentChainId[];
extern const char kBraveWalletUserAssets[];
// Added 10/2021 to migrate contract address to an empty string for ETH.
extern const char kBraveWalletUserAssetEthContractAddressMigrated[];
extern const char kBraveWalletAutoLockMinutes[];
extern const char kBraveWalletSelectedAccount[];
extern const char kSupportEip1559OnLocalhostChain[];

// DEPRECATED
extern const char kBraveWalletWeb3ProviderDeprecated[];
extern const char kDefaultWalletDeprecated[];
extern const char kBraveWalletPasswordEncryptorSalt[];
extern const char kBraveWalletPasswordEncryptorNonce[];
extern const char kBraveWalletEncryptedMnemonic[];
extern const char kBraveWalletDefaultKeyringAccountNum[];
extern const char kBraveWalletAccountNames[];
extern const char kBraveWalletBackupComplete[];

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PREF_NAMES_H_
