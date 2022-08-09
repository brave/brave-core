/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PREF_NAMES_H_

extern const char kDefaultEthereumWallet[];
extern const char kDefaultSolanaWallet[];
extern const char kDefaultBaseCurrency[];
extern const char kDefaultBaseCryptocurrency[];
extern const char kBraveWalletTransactions[];
extern const char kShowWalletIconOnToolbar[];
extern const char kShowWalletTestNetworks[];
extern const char kBraveWalletSelectedCoin[];
extern const char kBraveWalletLastUnlockTime[];
extern const char kBraveWalletPingReportedUnlockTime[];
extern const char kBraveWalletP3ALastReportTime[];
extern const char kBraveWalletP3AFirstReportTime[];
extern const char kBraveWalletP3AWeeklyStorage[];
extern const char kBraveWalletKeyrings[];
extern const char kBraveWalletCustomNetworks[];
extern const char kBraveWalletHiddenNetworks[];
extern const char kBraveWalletSelectedNetworks[];
extern const char kBraveWalletUserAssets[];
// Added 10/2021 to migrate contract address to an empty string for ETH.
extern const char kBraveWalletUserAssetEthContractAddressMigrated[];
// Added 06/2022 to add native assets of preloading networks to user assets.
extern const char kBraveWalletUserAssetsAddPreloadingNetworksMigrated[];
extern const char kBraveWalletAutoLockMinutes[];
extern const char kSupportEip1559OnLocalhostChain[];
// Added 02/2022 to migrate ethereum transactions to be under ethereum coin
// type.
extern const char kBraveWalletEthereumTransactionsCoinTypeMigrated[];

extern const char kBraveWalletP3AFirstUnlockTime[];
extern const char kBraveWalletP3ALastUnlockTime[];
extern const char kBraveWalletP3AUsedSecondDay[];

extern const char kBraveWalletWasOnboardingShown[];
extern const char kBraveWalletKeyringsNeedPasswordsMigration[];

// DEPRECATED
extern const char kBraveWalletSelectedAccount[];
extern const char kBraveWalletWeb3ProviderDeprecated[];
extern const char kDefaultWalletDeprecated[];
extern const char kBraveWalletCustomNetworksDeprecated[];
extern const char kBraveWalletCurrentChainId[];
extern const char kBraveWalletUserAssetsDeprecated[];
extern const char
    kBraveWalletUserAssetsAddPreloadingNetworksMigratedDeprecated[];

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PREF_NAMES_H_
