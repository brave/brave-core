/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/pref_names.h"

const char kShouldShowWalletSuggestionBadge[] =
    "brave.wallet.should_show_wallet_suggestion_badge";
const char kDefaultEthereumWallet[] = "brave.wallet.default_wallet2";
const char kDefaultSolanaWallet[] = "brave.wallet.default_solana_wallet";
const char kDefaultBaseCurrency[] = "brave.wallet.default_base_currency";
const char kDefaultBaseCryptocurrency[] =
    "brave.wallet.default_base_cryptocurrency";
const char kBraveWalletTransactions[] = "brave.wallet.transactions";
const char kShowWalletIconOnToolbar[] =
    "brave.wallet.show_wallet_icon_on_toolbar";
const char kBraveWalletLastUnlockTime[] =
    "brave.wallet.wallet_last_unlock_time_v2";
const char kBraveWalletPingReportedUnlockTime[] =
    "brave.wallet.wallet_report_unlock_time_ping";
const char kBraveWalletP3ALastReportTime[] =
    "brave.wallet.wallet_p3a_last_report_time";
const char kBraveWalletP3AFirstReportTime[] =
    "brave.wallet.wallet_p3a_first_report_time";
extern const char kBraveWalletP3ANFTGalleryUsed[] =
    "brave.wallet.wallet_p3a_nft_gallery_used";
const char kBraveWalletP3ANewUserBalanceReported[] =
    "brave.wallet.p3a_new_user_balance_reported";
const char kBraveWalletP3AWeeklyStorage[] =
    "brave.wallet.wallet_p3a_weekly_storage";
const char kBraveWalletP3AActiveWalletDict[] =
    "brave.wallet.wallet_p3a_active_wallets";
const char kBraveWalletCustomNetworks[] = "brave.wallet.custom_networks";
const char kBraveWalletHiddenNetworks[] = "brave.wallet.hidden_networks";
const char kBraveWalletSelectedNetworks[] = "brave.wallet.selected_networks";
const char kBraveWalletSelectedNetworksPerOrigin[] =
    "brave.wallet.selected_networks_origin";
const char kBraveWalletSelectedWalletAccount[] =
    "brave.wallet.selected_wallet_account";
const char kBraveWalletSelectedEthDappAccount[] =
    "brave.wallet.selected_eth_dapp_account";
const char kBraveWalletSelectedSolDappAccount[] =
    "brave.wallet.selected_sol_dapp_account";
const char kBraveWalletKeyrings[] = "brave.wallet.keyrings";
const char kBraveWalletUserAssets[] = "brave.wallet.wallet_user_assets";
const char kBraveWalletEthAllowancesCache[] =
    "brave.wallet.eth_allowances_cache";
const char kBraveWalletUserAssetEthContractAddressMigrated[] =
    "brave.wallet.user.asset.eth_contract_address_migrated";
const char kBraveWalletUserAssetsAddPreloadingNetworksMigrated[] =
    "brave.wallet.user.assets.add_preloading_networks_migrated_3";
const char kBraveWalletUserAssetsAddIsNFTMigrated[] =
    "brave.wallet.user.assets.add_is_nft_migrated";
const char kBraveWalletDefaultHiddenNetworksVersion[] =
    "brave.wallet.user.assets.default_hidden_networks_version";
const char kBraveWalletUserAssetsAddIsERC1155Migrated[] =
    "brave.wallet.user.assets.add_is_erc1155_migrated";
const char kBraveWalletDeprecateEthereumTestNetworksMigrated[] =
    "brave.wallet.deprecated_ethereum_test_networks_migrated";
const char kBraveWalletUserAssetsAddIsSpamMigrated[] =
    "brave.wallet.user.assets.add_is_spam_migrated";
const char kBraveWalletAutoLockMinutes[] = "brave.wallet.auto_lock_minutes";
const char kSupportEip1559OnLocalhostChain[] =
    "brave.wallet.support_eip1559_on_localhost_chain";
const char kBraveWalletEthereumTransactionsCoinTypeMigrated[] =
    "brave.wallet.ethereum_transactions.coin_type_migrated";
const char kBraveWalletP3AFirstUnlockTime[] =
    "brave.wallet.p3a_first_unlock_time";
const char kBraveWalletP3ALastUnlockTime[] =
    "brave.wallet.p3a_last_unlock_time";
const char kBraveWalletP3AUsedSecondDay[] = "brave.wallet.p3a_used_second_day";
const char kBraveWalletP3AOnboardingLastStep[] =
    "brave.wallet.p3a_last_onboarding_step";
const char kBraveWalletKeyringEncryptionKeysMigrated[] =
    "brave.wallet.keyring_encryption_keys_migrated";
const char kBraveWalletLastTransactionSentTimeDict[] =
    "brave.wallet.last_transaction_sent_time_dict";
const char kBraveWalletNftDiscoveryEnabled[] =
    "brave.wallet.nft_discovery_enabled";
const char kBraveWalletLastDiscoveredAssetsAt[] =
    "brave.wallet.last_discovered_assets_at";
const char kBraveWalletTransactionsChainIdMigrated[] =
    "brave.wallet.transactions.chain_id_migrated";
const char kBraveWalletSolanaTransactionsV0SupportMigrated[] =
    "brave.wallet.solana_transactions.v0_support_migrated";
const char kBraveWalletTransactionsFromPrefsToDBMigrated[] =
    "brave.wallet.transactions.from_prefs_to_db_migrated";
const char kBraveWalletCustomNetworksFantomMainnetMigrated[] =
    "brave.wallet.custom_networks.fantom_mainnet_migrated";

// DEPRECATED
const char kShowWalletTestNetworksDeprecated[] =
    "brave.wallet.show_wallet_test_networks";
const char kBraveWalletWeb3ProviderDeprecated[] = "brave.wallet.web3_provider";
const char kDefaultWalletDeprecated[] = "brave.wallet.default_wallet";
const char kBraveWalletCustomNetworksDeprecated[] =
    "brave.wallet.wallet_custom_networks";
const char kBraveWalletCurrentChainId[] =
    "brave.wallet.wallet_current_chain_id";
const char kBraveWalletUserAssetsDeprecated[] = "brave.wallet.user_assets";
const char kBraveWalletUserAssetsAddPreloadingNetworksMigratedDeprecated[] =
    "brave.wallet.user.assets.add_preloading_networks_migrated";
const char kBraveWalletUserAssetsAddPreloadingNetworksMigratedDeprecated2[] =
    "brave.wallet.user.assets.add_preloading_networks_migrated_2";
const char kPinnedNFTAssets[] = "brave.wallet.user_pin_data";
const char kAutoPinEnabled[] = "brave.wallet.auto_pin_enabled";
const char kBraveWalletSelectedCoinDeprecated[] = "brave.wallet.selected_coin";
