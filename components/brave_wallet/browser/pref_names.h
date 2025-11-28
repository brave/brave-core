/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PREF_NAMES_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PREF_NAMES_H_

#include "brave/components/brave_wallet/common/buildflags/buildflags.h"

static_assert(BUILDFLAG(ENABLE_BRAVE_WALLET));

namespace brave_wallet {

inline constexpr char kShouldShowWalletSuggestionBadge[] =
    "brave.wallet.should_show_wallet_suggestion_badge";
inline constexpr char kDefaultEthereumWallet[] = "brave.wallet.default_wallet2";
inline constexpr char kDefaultSolanaWallet[] =
    "brave.wallet.default_solana_wallet";
inline constexpr char kDefaultCardanoWallet[] =
    "brave.wallet.default_cardano_wallet";
inline constexpr char kDefaultBaseCurrency[] =
    "brave.wallet.default_base_currency";
inline constexpr char kDefaultBaseCryptocurrency[] =
    "brave.wallet.default_base_cryptocurrency";
inline constexpr char kShowWalletIconOnToolbar[] =
    "brave.wallet.show_wallet_icon_on_toolbar";
inline constexpr char kBraveWalletLastUnlockTime[] =
    "brave.wallet.wallet_last_unlock_time_v2";
inline constexpr char kBraveWalletPingReportedUnlockTime[] =
    "brave.wallet.wallet_report_unlock_time_ping";
inline constexpr char kBraveWalletP3ANFTGalleryUsed[] =
    "brave.wallet.wallet_p3a_nft_gallery_used";
inline constexpr char kBraveWalletP3ANewUserBalanceReported[] =
    "brave.wallet.p3a_new_user_balance_reported";
inline constexpr char kBraveWalletP3AActiveWalletDict[] =
    "brave.wallet.wallet_p3a_active_wallets";
inline constexpr char kBraveWalletCustomNetworks[] =
    "brave.wallet.custom_networks";
inline constexpr char kBraveWalletEip1559CustomChains[] =
    "brave.wallet.eip1559_chains";
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
inline constexpr char kBraveWalletSelectedAdaDappAccount[] =
    "brave.wallet.selected_ada_dapp_account";
inline constexpr char kBraveWalletKeyrings[] = "brave.wallet.keyrings";
inline constexpr char kBraveWalletUserAssetsList[] =
    "brave.wallet.wallet_user_assets_list";
inline constexpr char kBraveWalletEthAllowancesCache[] =
    "brave.wallet.eth_allowances_cache";
inline constexpr char kBraveWalletAutoLockMinutes[] =
    "brave.wallet.auto_lock_minutes";
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
inline constexpr char kBraveWalletPrivateWindowsEnabled[] =
    "brave.wallet.private_windows_enabled";
inline constexpr char kBraveWalletTransactionSimulationOptInStatus[] =
    "brave.wallet.transaction_simulation_opt_in_status";
inline constexpr char kBraveWalletEncryptorSalt[] =
    "brave.wallet.encryptor_salt";
inline constexpr char kBraveWalletMnemonic[] =
    "brave.wallet.encrypted_mnemonic";
inline constexpr char kBraveWalletLegacyEthSeedFormat[] =
    "brave.wallet.legacy_eth_seed_format";
inline constexpr char kBraveWalletMnemonicBackedUp[] =
    "brave.wallet.mnemonic_backed_up";

// Used to enable/disable Brave Wallet via a policy.
inline constexpr char kBraveWalletDisabledByPolicy[] =
    "brave.wallet.disabled_by_policy";

// Added 06/2024 to migrate Eip1559 flag to a separate pref.
inline constexpr char kBraveWalletEip1559ForCustomNetworksMigrated[] =
    "brave.wallet.eip1559_chains_migrated";
// 06/2024 to mark existing compressed Solana NFTs as compressed.
inline constexpr char kBraveWalletIsCompressedNftMigrated[] =
    "brave.wallet.is_compressed_nft_migrated";
// Added 11/2024 to mark Aurora mainnet as migrated.
inline constexpr char kBraveWalletAuroraMainnetMigrated[] =
    "brave.wallet.aurora_mainnet_migrated";

// 08/2024 to mark fix existing SPL tokens marked as unsupported
inline constexpr char kBraveWalletIsSPLTokenProgramMigrated[] =
    "brave.wallet.is_spl_token_program_migrated";

// 06/2024 migrated to kBraveWalletEip1559CustomChains.
inline constexpr char kSupportEip1559OnLocalhostChainDeprecated[] =
    "brave.wallet.support_eip1559_on_localhost_chain";
// Added 07/2024 to remove Goerli mainnet, previously a preloaded network.
inline constexpr char kBraveWalletGoerliNetworkMigrated[] =
    "brave.wallet.custom_networks.goerli_migrated";

// CryptoWallets prefs
// Deprecated 05/2025
inline constexpr char kERCAES256GCMSivNonceDeprecated[] =
    "brave.wallet.aes_256_gcm_siv_nonce";
inline constexpr char kERCEncryptedSeedDeprecated[] =
    "brave.wallet.encrypted_seed";
inline constexpr char kERCPrefVersionDeprecated[] = "brave.wallet.pref_version";
inline constexpr char kERCOptedIntoCryptoWalletsDeprecated[] =
    "brave.wallet.opted_in";

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_PREF_NAMES_H_
