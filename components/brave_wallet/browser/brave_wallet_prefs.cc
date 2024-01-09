/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"

#include <string>
#include <utility>
#include <vector>

#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/tx_state_manager.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/pref_names.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/pref_service_syncable.h"

namespace brave_wallet {

namespace {

constexpr int kDefaultWalletAutoLockMinutes = 10;

// Deprecated 12/2023.
constexpr char kBraveWalletUserAssetEthContractAddressMigrated[] =
    "brave.wallet.user.asset.eth_contract_address_migrated";
// Deprecated 12/2023.
constexpr char kBraveWalletUserAssetsAddPreloadingNetworksMigrated[] =
    "brave.wallet.user.assets.add_preloading_networks_migrated_3";
// Deprecated 12/2023.
constexpr char kBraveWalletUserAssetsAddIsNFTMigrated[] =
    "brave.wallet.user.assets.add_is_nft_migrated";
// Deprecated 12/2023.
constexpr char kBraveWalletEthereumTransactionsCoinTypeMigrated[] =
    "brave.wallet.ethereum_transactions.coin_type_migrated";
// Deprecated 12/2023.
constexpr char kBraveWalletDeprecateEthereumTestNetworksMigrated[] =
    "brave.wallet.deprecated_ethereum_test_networks_migrated";
// Deprecated 12/2023.
constexpr char kBraveWalletUserAssetsAddIsSpamMigrated[] =
    "brave.wallet.user.assets.add_is_spam_migrated";
// Deprecated 12/2023.
constexpr char kBraveWalletUserAssetsAddIsERC1155Migrated[] =
    "brave.wallet.user.assets.add_is_erc1155_migrated";

base::Value::Dict GetDefaultUserAssets() {
  base::Value::Dict user_assets_pref;
  user_assets_pref.Set(kEthereumPrefKey,
                       BraveWalletService::GetDefaultEthereumAssets());
  user_assets_pref.Set(kSolanaPrefKey,
                       BraveWalletService::GetDefaultSolanaAssets());
  user_assets_pref.Set(kFilecoinPrefKey,
                       BraveWalletService::GetDefaultFilecoinAssets());
  user_assets_pref.Set(kBitcoinPrefKey,
                       BraveWalletService::GetDefaultBitcoinAssets());
  user_assets_pref.Set(kZCashPrefKey,
                       BraveWalletService::GetDefaultZCashAssets());
  return user_assets_pref;
}

base::Value::Dict GetDefaultSelectedNetworks() {
  base::Value::Dict selected_networks;
  selected_networks.Set(kEthereumPrefKey, mojom::kMainnetChainId);
  selected_networks.Set(kSolanaPrefKey, mojom::kSolanaMainnet);
  selected_networks.Set(kFilecoinPrefKey, mojom::kFilecoinMainnet);
  selected_networks.Set(kBitcoinPrefKey, mojom::kBitcoinMainnet);
  selected_networks.Set(kZCashPrefKey, mojom::kZCashMainnet);

  return selected_networks;
}

base::Value::Dict GetDefaultSelectedNetworksPerOrigin() {
  base::Value::Dict selected_networks;
  selected_networks.Set(kEthereumPrefKey, base::Value::Dict());
  selected_networks.Set(kSolanaPrefKey, base::Value::Dict());
  selected_networks.Set(kFilecoinPrefKey, base::Value::Dict());
  selected_networks.Set(kBitcoinPrefKey, base::Value::Dict());
  selected_networks.Set(kZCashPrefKey, base::Value::Dict());

  return selected_networks;
}

base::Value::Dict GetDefaultHiddenNetworks() {
  base::Value::Dict hidden_networks;

  base::Value::List eth_hidden;
  eth_hidden.Append(mojom::kGoerliChainId);
  eth_hidden.Append(mojom::kSepoliaChainId);
  eth_hidden.Append(mojom::kLocalhostChainId);
  eth_hidden.Append(mojom::kFilecoinEthereumTestnetChainId);
  hidden_networks.Set(kEthereumPrefKey, std::move(eth_hidden));

  base::Value::List fil_hidden;
  fil_hidden.Append(mojom::kFilecoinTestnet);
  fil_hidden.Append(mojom::kLocalhostChainId);
  hidden_networks.Set(kFilecoinPrefKey, std::move(fil_hidden));

  base::Value::List sol_hidden;
  sol_hidden.Append(mojom::kSolanaDevnet);
  sol_hidden.Append(mojom::kSolanaTestnet);
  sol_hidden.Append(mojom::kLocalhostChainId);
  hidden_networks.Set(kSolanaPrefKey, std::move(sol_hidden));

  base::Value::List btc_hidden;
  btc_hidden.Append(mojom::kBitcoinTestnet);
  hidden_networks.Set(kBitcoinPrefKey, std::move(btc_hidden));

  base::Value::List zec_hidden;
  zec_hidden.Append(mojom::kZCashTestnet);
  hidden_networks.Set(kZCashPrefKey, std::move(zec_hidden));

  return hidden_networks;
}

void RegisterProfilePrefsDeprecatedMigrationFlags(
    user_prefs::PrefRegistrySyncable* registry) {
  // Deprecated 12/2023
  registry->RegisterBooleanPref(kBraveWalletUserAssetEthContractAddressMigrated,
                                false);
  // Deprecated 12/2023
  registry->RegisterBooleanPref(
      kBraveWalletUserAssetsAddPreloadingNetworksMigrated, false);
  // Deprecated 12/2023
  registry->RegisterBooleanPref(kBraveWalletUserAssetsAddIsNFTMigrated, false);
  // Deprecated 12/2023
  registry->RegisterBooleanPref(
      kBraveWalletEthereumTransactionsCoinTypeMigrated, false);
  // Deprecated 12/2023
  registry->RegisterBooleanPref(
      kBraveWalletDeprecateEthereumTestNetworksMigrated, false);
  // Deprecated 12/2023
  registry->RegisterBooleanPref(kBraveWalletUserAssetsAddIsSpamMigrated, false);
  // Deprecated 12/2023
  registry->RegisterBooleanPref(kBraveWalletUserAssetsAddIsERC1155Migrated,
                                false);
}

void ClearDeprecatedProfilePrefsMigrationFlags(PrefService* prefs) {
  // Deprecated 12/2023
  prefs->ClearPref(kBraveWalletUserAssetEthContractAddressMigrated);
  // Deprecated 12/2023
  prefs->ClearPref(kBraveWalletUserAssetsAddPreloadingNetworksMigrated);
  // Deprecated 12/2023
  prefs->ClearPref(kBraveWalletUserAssetsAddIsNFTMigrated);
  // Deprecated 12/2023
  prefs->ClearPref(kBraveWalletEthereumTransactionsCoinTypeMigrated);
  // Deprecated 12/2023
  prefs->ClearPref(kBraveWalletDeprecateEthereumTestNetworksMigrated);
  // Deprecated 12/2023
  prefs->ClearPref(kBraveWalletUserAssetsAddIsSpamMigrated);
  // Deprecated 12/2023
  prefs->ClearPref(kBraveWalletUserAssetsAddIsERC1155Migrated);
}

}  // namespace

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  registry->RegisterTimePref(kBraveWalletLastUnlockTime, base::Time());
  p3a_utils::RegisterFeatureUsagePrefs(
      registry, kBraveWalletP3AFirstUnlockTime, kBraveWalletP3ALastUnlockTime,
      kBraveWalletP3AUsedSecondDay, nullptr, nullptr);
  registry->RegisterBooleanPref(kBraveWalletP3ANewUserBalanceReported, false);
  registry->RegisterIntegerPref(kBraveWalletP3AOnboardingLastStep, 0);
  registry->RegisterBooleanPref(kBraveWalletP3ANFTGalleryUsed, false);
}

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kDisabledByPolicy, false);
  registry->RegisterIntegerPref(
      kDefaultEthereumWallet,
      static_cast<int>(
          brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension));
  registry->RegisterIntegerPref(
      kDefaultSolanaWallet,
      static_cast<int>(
          brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension));
  registry->RegisterStringPref(kDefaultBaseCurrency, "USD");
  registry->RegisterStringPref(kDefaultBaseCryptocurrency, "BTC");
  registry->RegisterBooleanPref(kShowWalletIconOnToolbar, true);
  registry->RegisterDictionaryPref(kBraveWalletTransactions);
  registry->RegisterDictionaryPref(kBraveWalletP3AActiveWalletDict);
  registry->RegisterDictionaryPref(kBraveWalletKeyrings);
  registry->RegisterBooleanPref(kBraveWalletKeyringEncryptionKeysMigrated,
                                false);
  registry->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  registry->RegisterDictionaryPref(kBraveWalletHiddenNetworks,
                                   GetDefaultHiddenNetworks());
  registry->RegisterDictionaryPref(kBraveWalletSelectedNetworks,
                                   GetDefaultSelectedNetworks());
  registry->RegisterDictionaryPref(kBraveWalletSelectedNetworksPerOrigin,
                                   GetDefaultSelectedNetworksPerOrigin());
  registry->RegisterDictionaryPref(kBraveWalletUserAssets,
                                   GetDefaultUserAssets());
  registry->RegisterIntegerPref(kBraveWalletAutoLockMinutes,
                                kDefaultWalletAutoLockMinutes);
  registry->RegisterDictionaryPref(kBraveWalletEthAllowancesCache);
  registry->RegisterBooleanPref(kSupportEip1559OnLocalhostChain, false);
  registry->RegisterDictionaryPref(kBraveWalletLastTransactionSentTimeDict);
  registry->RegisterTimePref(kBraveWalletLastDiscoveredAssetsAt, base::Time());

  registry->RegisterDictionaryPref(kPinnedNFTAssets);
  registry->RegisterBooleanPref(kAutoPinEnabled, false);
  registry->RegisterBooleanPref(kShouldShowWalletSuggestionBadge, true);
  registry->RegisterBooleanPref(kBraveWalletNftDiscoveryEnabled, false);

  registry->RegisterStringPref(kBraveWalletSelectedWalletAccount, "");
  registry->RegisterStringPref(kBraveWalletSelectedEthDappAccount, "");
  registry->RegisterStringPref(kBraveWalletSelectedSolDappAccount, "");
}

void RegisterLocalStatePrefsForMigration(PrefRegistrySimple* registry) {
  // Added 04/2023
  registry->RegisterTimePref(kBraveWalletP3ALastReportTimeDeprecated,
                             base::Time());
  registry->RegisterTimePref(kBraveWalletP3AFirstReportTimeDeprecated,
                             base::Time());
  registry->RegisterListPref(kBraveWalletP3AWeeklyStorageDeprecated);
}

void RegisterProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  RegisterProfilePrefsDeprecatedMigrationFlags(registry);

  // Added 04/2023
  p3a_utils::RegisterFeatureUsagePrefs(
      registry, kBraveWalletP3AFirstUnlockTime, kBraveWalletP3ALastUnlockTime,
      kBraveWalletP3AUsedSecondDay, nullptr, nullptr);
  registry->RegisterTimePref(kBraveWalletLastUnlockTime, base::Time());
  registry->RegisterTimePref(kBraveWalletP3ALastReportTimeDeprecated,
                             base::Time());
  registry->RegisterTimePref(kBraveWalletP3AFirstReportTimeDeprecated,
                             base::Time());
  registry->RegisterListPref(kBraveWalletP3AWeeklyStorageDeprecated);

  // Added 02/2023
  registry->RegisterBooleanPref(kBraveWalletTransactionsChainIdMigrated, false);

  // Added 03/2023
  registry->RegisterIntegerPref(kBraveWalletDefaultHiddenNetworksVersion, 0);

  // Added 04/2023
  registry->RegisterBooleanPref(kBraveWalletSolanaTransactionsV0SupportMigrated,
                                false);

  // Added 06/2023
  registry->RegisterIntegerPref(
      kBraveWalletSelectedCoinDeprecated,
      static_cast<int>(brave_wallet::mojom::CoinType::ETH));

  // Added 07/2023
  registry->RegisterBooleanPref(kBraveWalletTransactionsFromPrefsToDBMigrated,
                                false);

  // Added 08/2023
  registry->RegisterBooleanPref(kBraveWalletCustomNetworksFantomMainnetMigrated,
                                false);
}

void ClearJsonRpcServiceProfilePrefs(PrefService* prefs) {
  DCHECK(prefs);
  prefs->ClearPref(kBraveWalletCustomNetworks);
  prefs->ClearPref(kBraveWalletHiddenNetworks);
  prefs->ClearPref(kBraveWalletSelectedNetworks);
  prefs->ClearPref(kBraveWalletSelectedNetworksPerOrigin);
  prefs->ClearPref(kSupportEip1559OnLocalhostChain);
}

void ClearKeyringServiceProfilePrefs(PrefService* prefs) {
  DCHECK(prefs);
  prefs->ClearPref(kBraveWalletKeyrings);
  prefs->ClearPref(kBraveWalletAutoLockMinutes);
  prefs->ClearPref(kBraveWalletSelectedWalletAccount);
  prefs->ClearPref(kBraveWalletSelectedEthDappAccount);
  prefs->ClearPref(kBraveWalletSelectedSolDappAccount);
}

void ClearTxServiceProfilePrefs(PrefService* prefs) {
  DCHECK(prefs);
  // Remove this when we remove kBraveWalletTransactions.
  prefs->ClearPref(kBraveWalletTransactions);
}

void ClearBraveWalletServicePrefs(PrefService* prefs) {
  DCHECK(prefs);
  prefs->ClearPref(kBraveWalletUserAssets);
  prefs->ClearPref(kDefaultBaseCurrency);
  prefs->ClearPref(kDefaultBaseCryptocurrency);
  prefs->ClearPref(kBraveWalletEthAllowancesCache);
}

void MigrateObsoleteProfilePrefs(PrefService* prefs) {
  ClearDeprecatedProfilePrefsMigrationFlags(prefs);

  // Added 03/2023 to add filecoin evm support.
  BraveWalletService::MigrateHiddenNetworks(prefs);

  // Added 08/2023 to add Fantom as a custom network if selected for the default
  // or custom origins.
  BraveWalletService::MigrateFantomMainnetAsCustomNetwork(prefs);

  // Added 02/2023
  TxStateManager::MigrateAddChainIdToTransactionInfo(prefs);

  // Added 07/2023
  KeyringService::MigrateDerivedAccountIndex(prefs);
}

}  // namespace brave_wallet
