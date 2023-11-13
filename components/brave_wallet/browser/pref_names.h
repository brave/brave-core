/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PREF_NAMES_H_

inline constexpr char kShouldShowWalletSuggestionBadge[] =
    "brave.wallet.should_show_wallet_suggestion_badge";
inline constexpr char kDefaultEthereumWallet[] = "brave.wallet.default_wallet2";
inline constexpr char kDefaultSolanaWallet[] =
    "brave.wallet.default_solana_wallet";
inline constexpr char kDefaultBaseCurrency[] =
    "brave.wallet.default_base_currency";
inline constexpr char kDefaultBaseCryptocurrency[] =
    "brave.wallet.default_base_cryptocurrency";
inline constexpr char kBraveWalletTransactions[] = "brave.wallet.transactions";
inline constexpr char kShowWalletIconOnToolbar[] =
    "brave.wallet.show_wallet_icon_on_toolbar";
inline constexpr char kBraveWalletLastUnlockTime[] =
    "brave.wallet.wallet_last_unlock_time_v2";
inline constexpr char kBraveWalletPingReportedUnlockTime[] =
    "brave.wallet.wallet_report_unlock_time_ping";
inline constexpr char kBraveWalletP3ALastReportTime[] =
    "brave.wallet.wallet_p3a_last_report_time";
inline constexpr char kBraveWalletP3AFirstReportTime[] =
    "brave.wallet.wallet_p3a_first_report_time";
inline constexpr char kBraveWalletP3ANFTGalleryUsed[] =
    "brave.wallet.wallet_p3a_nft_gallery_used";
inline constexpr char kBraveWalletP3ANewUserBalanceReported[] =
    "brave.wallet.p3a_new_user_balance_reported";
inline constexpr char kBraveWalletP3AWeeklyStorage[] =
    "brave.wallet.wallet_p3a_weekly_storage";
inline constexpr char kBraveWalletP3AActiveWalletDict[] =
    "brave.wallet.wallet_p3a_active_wallets";
inline constexpr char kBraveWalletCustomNetworks[] =
    "brave.wallet.custom_networks";
inline constexpr char kBraveWalletHiddenNetworks[] =
    "brave.wallet.hidden_networks";
inline constexpr char kBraveWalletSelectedNetworks[] =
    "brave.wallet.selected_networks";
inline constexpr char kBraveWalletSelectedNetworksPerOrigin[] =
    "brave.wallet.selected_networks_origin";
inline constexpr char kBraveWalletSelectedWalletAccount[] =
    "brave.wallet.selected_wallet_account";
inline constexpr char kBraveWalletSelectedEthDappAccount[] =
    "brave.wallet.selected_eth_dapp_account";
inline constexpr char kBraveWalletSelectedSolDappAccount[] =
    "brave.wallet.selected_sol_dapp_account";
inline constexpr char kBraveWalletKeyrings[] = "brave.wallet.keyrings";
inline constexpr char kBraveWalletUserAssets[] =
    "brave.wallet.wallet_user_assets";
inline constexpr char kBraveWalletEthAllowancesCache[] =
    "brave.wallet.eth_allowances_cache";
// Added 10/2021 to migrate contract address to an empty string for ETH.
inline constexpr char kBraveWalletUserAssetEthContractAddressMigrated[] =
    "brave.wallet.user.asset.eth_contract_address_migrated";
// Added 06/2022 to add native assets of preloading networks to user assets.
inline constexpr char kBraveWalletUserAssetsAddPreloadingNetworksMigrated[] =
    "brave.wallet.user.assets.add_preloading_networks_migrated_3";
// Added 10/2022 to set is_nft = true for existing ERC721 tokens.
inline constexpr char kBraveWalletUserAssetsAddIsNFTMigrated[] =
    "brave.wallet.user.assets.add_is_nft_migrated";
// Added 03/2023 to add networks hidden by default
inline constexpr char kBraveWalletDefaultHiddenNetworksVersion[] =
    "brave.wallet.user.assets.default_hidden_networks_version";
// Added 03/2023 to set is_erc1155 = false for all existing tokens.
inline constexpr char kBraveWalletUserAssetsAddIsERC1155Migrated[] =
    "brave.wallet.user.assets.add_is_erc1155_migrated";
// Added 10/2022 to replace ETH selected network with mainnet if selected
// network is one of the Ethereum testnets deprecated on 10/5/2022.
inline constexpr char kBraveWalletDeprecateEthereumTestNetworksMigrated[] =
    "brave.wallet.deprecated_ethereum_test_networks_migrated";
// Added 06/2023 to set is_spam = false for all existing tokens.
inline constexpr char kBraveWalletUserAssetsAddIsSpamMigrated[] =
    "brave.wallet.user.assets.add_is_spam_migrated";
inline constexpr char kBraveWalletAutoLockMinutes[] =
    "brave.wallet.auto_lock_minutes";
inline constexpr char kSupportEip1559OnLocalhostChain[] =
    "brave.wallet.support_eip1559_on_localhost_chain";
// Added 02/2022 to migrate ethereum transactions to be under ethereum coin
// type.
inline constexpr char kBraveWalletEthereumTransactionsCoinTypeMigrated[] =
    "brave.wallet.ethereum_transactions.coin_type_migrated";
inline constexpr char kBraveWalletP3AFirstUnlockTime[] =
    "brave.wallet.p3a_first_unlock_time";
inline constexpr char kBraveWalletP3ALastUnlockTime[] =
    "brave.wallet.p3a_last_unlock_time";
inline constexpr char kBraveWalletP3AUsedSecondDay[] =
    "brave.wallet.p3a_used_second_day";
inline constexpr char kBraveWalletP3AOnboardingLastStep[] =
    "brave.wallet.p3a_last_onboarding_step";
inline constexpr char kBraveWalletKeyringEncryptionKeysMigrated[] =
    "brave.wallet.keyring_encryption_keys_migrated";
inline constexpr char kBraveWalletLastTransactionSentTimeDict[] =
    "brave.wallet.last_transaction_sent_time_dict";
inline constexpr char kBraveWalletNftDiscoveryEnabled[] =
    "brave.wallet.nft_discovery_enabled";
inline constexpr char kBraveWalletLastDiscoveredAssetsAt[] =
    "brave.wallet.last_discovered_assets_at";
// Added 02/2023 to migrate transactions to contain the
// chain_id for each one.
inline constexpr char kBraveWalletTransactionsChainIdMigrated[] =
    "brave.wallet.transactions.chain_id_migrated";
// Added 04/2023 to migrate solana transactions for v0 transaction support.
inline constexpr char kBraveWalletSolanaTransactionsV0SupportMigrated[] =
    "brave.wallet.solana_transactions.v0_support_migrated";
// Added 07/2023 to migrate transactions from prefs to DB.
inline constexpr char kBraveWalletTransactionsFromPrefsToDBMigrated[] =
    "brave.wallet.transactions.from_prefs_to_db_migrated";

// Added 08/09 to migrate Fantom mainnet, previously a preloaded network,
// to a custom network.
inline constexpr char kBraveWalletCustomNetworksFantomMainnetMigrated[] =
    "brave.wallet.custom_networks.fantom_mainnet_migrated";

// DEPRECATED
inline constexpr char kShowWalletTestNetworksDeprecated[] =
    "brave.wallet.show_wallet_test_networks";
inline constexpr char kBraveWalletWeb3ProviderDeprecated[] =
    "brave.wallet.web3_provider";
inline constexpr char kDefaultWalletDeprecated[] =
    "brave.wallet.default_wallet";
inline constexpr char kBraveWalletCustomNetworksDeprecated[] =
    "brave.wallet.wallet_custom_networks";
inline constexpr char kBraveWalletCurrentChainId[] =
    "brave.wallet.wallet_current_chain_id";
inline constexpr char kBraveWalletUserAssetsDeprecated[] =
    "brave.wallet.user_assets";
inline constexpr char
    kBraveWalletUserAssetsAddPreloadingNetworksMigratedDeprecated[] =
        "brave.wallet.user.assets.add_preloading_networks_migrated";
inline constexpr char
    kBraveWalletUserAssetsAddPreloadingNetworksMigratedDeprecated2[] =
        "brave.wallet.user.assets.add_preloading_networks_migrated_2";
inline constexpr char kPinnedNFTAssets[] = "brave.wallet.user_pin_data";
inline constexpr char kAutoPinEnabled[] = "brave.wallet.auto_pin_enabled";
// 06/2023
inline constexpr char kBraveWalletSelectedCoinDeprecated[] =
    "brave.wallet.selected_coin";

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PREF_NAMES_H_
