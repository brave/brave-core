/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/pref_names.h"

#include <utility>

#include "base/check.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/keyring_service_migrations.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_wallet {

namespace {

constexpr int kDefaultWalletAutoLockMinutes = 10;

// Added 05/2026
inline constexpr char kBraveWalletEip1559ForCustomNetworksMigrated[] =
    "brave.wallet.eip1559_chains_migrated";
// Added 05/2026
inline constexpr char kBraveWalletIsCompressedNftMigrated[] =
    "brave.wallet.is_compressed_nft_migrated";
// Added 05/2026
inline constexpr char kBraveWalletAuroraMainnetMigrated[] =
    "brave.wallet.aurora_mainnet_migrated";
// Added 05/2026
inline constexpr char kBraveWalletIsSPLTokenProgramMigrated[] =
    "brave.wallet.is_spl_token_program_migrated";
// Added 05/2026
inline constexpr char kBraveWalletGoerliNetworkMigrated[] =
    "brave.wallet.custom_networks.goerli_migrated";

// Deprecated 05/2026
inline constexpr char kBraveWalletP3ANewUserBalanceReportedDeprecated[] =
    "brave.wallet.p3a_new_user_balance_reported";
// Deprecated 05/2026
inline constexpr char kBraveWalletP3AActiveWalletDictDeprecated[] =
    "brave.wallet.wallet_p3a_active_wallets";
// Deprecated 05/2026
inline constexpr char kBraveWalletLastTransactionSentTimeDictDeprecated[] =
    "brave.wallet.last_transaction_sent_time_dict";
// Deprecated 05/2026
inline constexpr char kBraveWalletP3ANFTGalleryUsedDeprecated[] =
    "brave.wallet.wallet_p3a_nft_gallery_used";
// Deprecated 05/2026
inline constexpr char kBraveWalletP3AOnboardingLastStepDeprecated[] =
    "brave.wallet.p3a_last_onboarding_step";

base::DictValue GetDefaultSelectedNetworks() {
  base::DictValue selected_networks;
  selected_networks.Set(kEthereumPrefKey, mojom::kMainnetChainId);
  selected_networks.Set(kSolanaPrefKey, mojom::kSolanaMainnet);
  selected_networks.Set(kFilecoinPrefKey, mojom::kFilecoinMainnet);
  selected_networks.Set(kBitcoinPrefKey, mojom::kBitcoinMainnet);
  selected_networks.Set(kZCashPrefKey, mojom::kZCashMainnet);

  return selected_networks;
}

base::DictValue GetDefaultSelectedNetworksPerOrigin() {
  base::DictValue selected_networks;
  selected_networks.Set(kEthereumPrefKey, base::DictValue());
  selected_networks.Set(kSolanaPrefKey, base::DictValue());
  selected_networks.Set(kFilecoinPrefKey, base::DictValue());
  selected_networks.Set(kBitcoinPrefKey, base::DictValue());
  selected_networks.Set(kZCashPrefKey, base::DictValue());

  return selected_networks;
}

base::DictValue GetDefaultHiddenNetworks() {
  base::DictValue hidden_networks;

  base::ListValue eth_hidden;
  eth_hidden.Append(mojom::kSepoliaChainId);
  eth_hidden.Append(mojom::kLocalhostChainId);
  eth_hidden.Append(mojom::kFilecoinEthereumTestnetChainId);
  hidden_networks.Set(kEthereumPrefKey, std::move(eth_hidden));

  base::ListValue fil_hidden;
  fil_hidden.Append(mojom::kFilecoinTestnet);
  fil_hidden.Append(mojom::kLocalhostChainId);
  hidden_networks.Set(kFilecoinPrefKey, std::move(fil_hidden));

  base::ListValue sol_hidden;
  sol_hidden.Append(mojom::kSolanaDevnet);
  sol_hidden.Append(mojom::kSolanaTestnet);
  sol_hidden.Append(mojom::kLocalhostChainId);
  hidden_networks.Set(kSolanaPrefKey, std::move(sol_hidden));

  base::ListValue btc_hidden;
  btc_hidden.Append(mojom::kBitcoinTestnet);
  hidden_networks.Set(kBitcoinPrefKey, std::move(btc_hidden));

  base::ListValue zec_hidden;
  zec_hidden.Append(mojom::kZCashTestnet);
  hidden_networks.Set(kZCashPrefKey, std::move(zec_hidden));

  base::ListValue cardano_hidden;
  cardano_hidden.Append(mojom::kCardanoTestnet);
  hidden_networks.Set(kCardanoPrefKey, std::move(cardano_hidden));

  base::ListValue polkadot_hidden;
  polkadot_hidden.Append(mojom::kPolkadotTestnet);
  hidden_networks.Set(kPolkadotPrefKey, std::move(polkadot_hidden));

  return hidden_networks;
}

}  // namespace

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimePref(kBraveWalletLastUnlockTime, base::Time());
  p3a_utils::RegisterFeatureUsagePrefs(
      registry, kBraveWalletP3AFirstUnlockTime, kBraveWalletP3ALastUnlockTime,
      kBraveWalletP3AUsedSecondDay, nullptr, nullptr);
}

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(kBraveWalletDisabledByPolicy, false);
  registry->RegisterIntegerPref(
      kDefaultEthereumWallet,
      static_cast<int>(
          brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension));
  registry->RegisterIntegerPref(
      kDefaultSolanaWallet,
      static_cast<int>(
          brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension));
  registry->RegisterIntegerPref(
      kDefaultCardanoWallet,
      static_cast<int>(brave_wallet::mojom::DefaultWallet::BraveWallet));
  registry->RegisterStringPref(kDefaultBaseCurrency, "USD");
  registry->RegisterStringPref(kDefaultBaseCryptocurrency, "BTC");
  registry->RegisterBooleanPref(kShowWalletIconOnToolbar, true);
  registry->RegisterDictionaryPref(kBraveWalletKeyrings);
  registry->RegisterBooleanPref(kBraveWalletKeyringEncryptionKeysMigrated,
                                false);
  registry->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  registry->RegisterDictionaryPref(kBraveWalletEip1559CustomChains);
  registry->RegisterDictionaryPref(kBraveWalletHiddenNetworks,
                                   GetDefaultHiddenNetworks());
  registry->RegisterListPref(kBraveWalletHiddenAccounts);
  registry->RegisterDictionaryPref(kBraveWalletSelectedNetworks,
                                   GetDefaultSelectedNetworks());
  registry->RegisterDictionaryPref(kBraveWalletSelectedNetworksPerOrigin,
                                   GetDefaultSelectedNetworksPerOrigin());
  registry->RegisterListPref(kBraveWalletUserAssetsList,
                             GetDefaultUserAssets());
  registry->RegisterIntegerPref(kBraveWalletAutoLockMinutes,
                                kDefaultWalletAutoLockMinutes);
  registry->RegisterDictionaryPref(kBraveWalletEthAllowancesCache);
  registry->RegisterDictionaryPref(kBraveWalletPolkadotChainMetadata);
  registry->RegisterTimePref(kBraveWalletLastDiscoveredAssetsAt, base::Time());

  registry->RegisterBooleanPref(kShouldShowWalletSuggestionBadge, true);
  registry->RegisterBooleanPref(kBraveWalletNftDiscoveryEnabled, false);
  registry->RegisterBooleanPref(kBraveWalletPrivateWindowsEnabled, false);

  registry->RegisterStringPref(kBraveWalletSelectedWalletAccount, "");
  registry->RegisterStringPref(kBraveWalletSelectedEthDappAccount, "");
  registry->RegisterStringPref(kBraveWalletSelectedSolDappAccount, "");
  registry->RegisterStringPref(kBraveWalletSelectedAdaDappAccount, "");

  registry->RegisterIntegerPref(
      kBraveWalletTransactionSimulationOptInStatus,
      static_cast<int>(brave_wallet::mojom::BlowfishOptInStatus::kUnset));
  registry->RegisterStringPref(kBraveWalletEncryptorSalt, "");
  registry->RegisterDictionaryPref(kBraveWalletMnemonic);
  registry->RegisterBooleanPref(kBraveWalletLegacyEthSeedFormat, false);
  registry->RegisterBooleanPref(kBraveWalletMnemonicBackedUp, false);

  // Register Deprecated CryptoWallet prefs
  // We can eventually remove these. Code removed 05/2025
  registry->RegisterIntegerPref(kERCPrefVersionDeprecated, 0);
  registry->RegisterStringPref(kERCAES256GCMSivNonceDeprecated, "");
  registry->RegisterStringPref(kERCEncryptedSeedDeprecated, "");
  registry->RegisterBooleanPref(kERCOptedIntoCryptoWalletsDeprecated, false);
}

void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry) {
  // Deprecated 05/2026
  registry->RegisterBooleanPref(kBraveWalletP3ANewUserBalanceReportedDeprecated,
                                false);
  // Deprecated 05/2026
  registry->RegisterBooleanPref(kBraveWalletP3ANFTGalleryUsedDeprecated, false);
  // Deprecated 05/2026
  registry->RegisterIntegerPref(kBraveWalletP3AOnboardingLastStepDeprecated, 0);
}

void MigrateObsoleteLocalStatePrefs(PrefService* local_state) {
  // Deprecated 05/2026
  local_state->ClearPref(kBraveWalletP3ANewUserBalanceReportedDeprecated);
  // Deprecated 05/2026
  local_state->ClearPref(kBraveWalletP3ANFTGalleryUsedDeprecated);
  // Deprecated 05/2026
  local_state->ClearPref(kBraveWalletP3AOnboardingLastStepDeprecated);
}

void RegisterProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  // Added 05/2026
  registry->RegisterBooleanPref(kBraveWalletEip1559ForCustomNetworksMigrated,
                                false);
  // Added 05/2026
  registry->RegisterBooleanPref(kBraveWalletIsCompressedNftMigrated, false);
  // Added 05/2026
  registry->RegisterBooleanPref(kBraveWalletGoerliNetworkMigrated, false);
  // Added 05/2026
  registry->RegisterBooleanPref(kBraveWalletIsSPLTokenProgramMigrated, false);
  // Added 05/2026
  registry->RegisterBooleanPref(kBraveWalletAuroraMainnetMigrated, false);

  // Added 05/2026
  registry->RegisterDictionaryPref(kBraveWalletP3AActiveWalletDictDeprecated);

  // Added 05/2026
  registry->RegisterDictionaryPref(
      kBraveWalletLastTransactionSentTimeDictDeprecated);
}

void ClearJsonRpcServiceProfilePrefs(PrefService* prefs) {
  DCHECK(prefs);
  prefs->ClearPref(kBraveWalletCustomNetworks);
  prefs->ClearPref(kBraveWalletHiddenNetworks);
  prefs->ClearPref(kBraveWalletSelectedNetworks);
  prefs->ClearPref(kBraveWalletSelectedNetworksPerOrigin);
  prefs->ClearPref(kBraveWalletEip1559CustomChains);
}

void ClearKeyringServiceProfilePrefs(PrefService* prefs) {
  DCHECK(prefs);
  prefs->ClearPref(kBraveWalletKeyrings);
  prefs->ClearPref(kBraveWalletEncryptorSalt);
  prefs->ClearPref(kBraveWalletMnemonic);
  prefs->ClearPref(kBraveWalletLegacyEthSeedFormat);
  prefs->ClearPref(kBraveWalletMnemonicBackedUp);
  prefs->ClearPref(kBraveWalletAutoLockMinutes);
  prefs->ClearPref(kBraveWalletSelectedWalletAccount);
  prefs->ClearPref(kBraveWalletSelectedEthDappAccount);
  prefs->ClearPref(kBraveWalletSelectedSolDappAccount);
  prefs->ClearPref(kBraveWalletSelectedAdaDappAccount);
  prefs->ClearPref(kBraveWalletHiddenAccounts);
}

void ClearBraveWalletServicePrefs(PrefService* prefs) {
  DCHECK(prefs);
  prefs->ClearPref(kBraveWalletUserAssetsList);
  prefs->ClearPref(kDefaultBaseCurrency);
  prefs->ClearPref(kDefaultBaseCryptocurrency);
  prefs->ClearPref(kBraveWalletEthAllowancesCache);
}

void MigrateCryptoWalletsPrefToBraveWallet(PrefService* prefs) {
  int value = prefs->GetInteger(kDefaultEthereumWallet);
  if (value ==
      static_cast<int>(mojom::DefaultWallet::CryptoWalletsDeprecated)) {
    prefs->SetInteger(
        kDefaultEthereumWallet,
        static_cast<int>(mojom::DefaultWallet::BraveWalletPreferExtension));
  }
}

void MigrateObsoleteProfilePrefs(PrefService* prefs) {
  // Added 07/2023
  MigrateDerivedAccountIndex(prefs);

  // CryptoWallets Removed 05/2025
  MigrateCryptoWalletsPrefToBraveWallet(prefs);

  // Added 05/2026
  prefs->ClearPref(kBraveWalletP3AActiveWalletDictDeprecated);

  // Added 05/2026
  prefs->ClearPref(kBraveWalletLastTransactionSentTimeDictDeprecated);

  // Added 05/2026
  prefs->ClearPref(kBraveWalletEip1559ForCustomNetworksMigrated);
  // Added 05/2026
  prefs->ClearPref(kBraveWalletIsCompressedNftMigrated);
  // Added 05/2026
  prefs->ClearPref(kBraveWalletGoerliNetworkMigrated);
  // Added 05/2026
  prefs->ClearPref(kBraveWalletIsSPLTokenProgramMigrated);
  // Added 05/2026
  prefs->ClearPref(kBraveWalletAuroraMainnetMigrated);
}

}  // namespace brave_wallet
