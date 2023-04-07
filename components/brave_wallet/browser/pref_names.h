/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PREF_NAMES_H_

extern const char kShouldShowWalletSuggestionBadge[];
extern const char kDefaultEthereumWallet[];
extern const char kDefaultSolanaWallet[];
extern const char kDefaultBaseCurrency[];
extern const char kDefaultBaseCryptocurrency[];
extern const char kBraveWalletTransactions[];
extern const char kShowWalletIconOnToolbar[];
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
// Added 10/2022 to set is_nft = true for existing ERC721 tokens.
extern const char kBraveWalletUserAssetsAddIsNFTMigrated[];
// Added 03/2023 to add networks hidden by default
extern const char kBraveWalletDefaultHiddenNetworksVersion[];
// Added 03/2023 to set is_erc1155 = false for all existing tokens.
extern const char kBraveWalletUserAssetsAddIsERC1155Migrated[];
// Added 10/2022 to replace ETH selected network with mainnet if selected
// network is one of the Ethereum testnets deprecated on 10/5/2022.
extern const char kBraveWalletDeprecateEthereumTestNetworksMigrated[];
extern const char kBraveWalletAutoLockMinutes[];
extern const char kSupportEip1559OnLocalhostChain[];
// Added 02/2022 to migrate ethereum transactions to be under ethereum coin
// type.
extern const char kBraveWalletEthereumTransactionsCoinTypeMigrated[];

extern const char kBraveWalletP3AFirstUnlockTime[];
extern const char kBraveWalletP3ALastUnlockTime[];
extern const char kBraveWalletP3AUsedSecondDay[];

extern const char kBraveWalletP3AActiveWalletDict[];
extern const char kBraveWalletKeyringEncryptionKeysMigrated[];
extern const char kBraveWalletNftDiscoveryEnabled[];
extern const char kBraveWalletLastDiscoveredAssetsAt[];

extern const char kBraveWalletLastTransactionSentTimeDict[];
// Added 02/2023 to migrate transactions to contain the
// chain_id for each one.
extern const char kBraveWalletTransactionsChainIdMigrated[];
// Added 04/2023 to migrate solana transactions for v0 transaction support.
extern const char kBraveWalletSolanaTransactionsV0SupportMigrated[];

// DEPRECATED
extern const char kShowWalletTestNetworksDeprecated[];
extern const char kBraveWalletSelectedAccount[];
extern const char kBraveWalletWeb3ProviderDeprecated[];
extern const char kDefaultWalletDeprecated[];
extern const char kBraveWalletCustomNetworksDeprecated[];
extern const char kBraveWalletCurrentChainId[];
extern const char kBraveWalletUserAssetsDeprecated[];
extern const char
    kBraveWalletUserAssetsAddPreloadingNetworksMigratedDeprecated[];
extern const char
    kBraveWalletUserAssetsAddPreloadingNetworksMigratedDeprecated2[];
extern const char kPinnedNFTAssets[];
extern const char kAutoPinEnabled[];

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PREF_NAMES_H_
