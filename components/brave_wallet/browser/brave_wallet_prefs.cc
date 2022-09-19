/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"

#include <string>
#include <utility>
#include <vector>

#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/pref_names.h"
#include "brave/components/p3a_utils/feature_usage.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/pref_service_syncable.h"

namespace brave_wallet {

namespace {

base::Value::Dict GetDefaultUserAssets() {
  base::Value::Dict user_assets_pref;
  user_assets_pref.Set(kEthereumPrefKey,
                       BraveWalletService::GetDefaultEthereumAssets());
  user_assets_pref.Set(kSolanaPrefKey,
                       BraveWalletService::GetDefaultSolanaAssets());
  user_assets_pref.Set(kFilecoinPrefKey,
                       BraveWalletService::GetDefaultFilecoinAssets());
  return user_assets_pref;
}

base::Value::Dict GetDefaultSelectedNetworks() {
  base::Value::Dict selected_networks;
  selected_networks.Set(kEthereumPrefKey, mojom::kMainnetChainId);
  selected_networks.Set(kSolanaPrefKey, mojom::kSolanaMainnet);
  selected_networks.Set(kFilecoinPrefKey, mojom::kFilecoinMainnet);

  return selected_networks;
}

}  // namespace

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
  registry->RegisterBooleanPref(kShowWalletTestNetworks, false);
  registry->RegisterIntegerPref(
      kBraveWalletSelectedCoin,
      static_cast<int>(brave_wallet::mojom::CoinType::ETH));
  registry->RegisterDictionaryPref(kBraveWalletTransactions);
  registry->RegisterTimePref(kBraveWalletLastUnlockTime, base::Time());
  registry->RegisterTimePref(kBraveWalletP3ALastReportTime, base::Time());
  registry->RegisterTimePref(kBraveWalletP3AFirstReportTime, base::Time());
  registry->RegisterListPref(kBraveWalletP3AWeeklyStorage);
  registry->RegisterDictionaryPref(kBraveWalletKeyrings);
  registry->RegisterBooleanPref(kBraveWalletKeyringEncryptionKeysMigrated,
                                false);
  registry->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  registry->RegisterDictionaryPref(kBraveWalletHiddenNetworks);
  registry->RegisterDictionaryPref(kBraveWalletSelectedNetworks,
                                   base::Value(GetDefaultSelectedNetworks()));
  registry->RegisterDictionaryPref(kBraveWalletUserAssets,
                                   base::Value(GetDefaultUserAssets()));
  registry->RegisterIntegerPref(kBraveWalletAutoLockMinutes, 5);
  registry->RegisterStringPref(kBraveWalletSelectedAccount, "");
  registry->RegisterBooleanPref(kSupportEip1559OnLocalhostChain, false);
  p3a_utils::RegisterFeatureUsagePrefs(registry, kBraveWalletP3AFirstUnlockTime,
                                       kBraveWalletP3ALastUnlockTime,
                                       kBraveWalletP3AUsedSecondDay, nullptr);
  registry->RegisterBooleanPref(kBraveWalletWasOnboardingShown, false);
}

void RegisterProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  // Added 10/2021
  registry->RegisterBooleanPref(kBraveWalletUserAssetEthContractAddressMigrated,
                                false);
  // Added 09/2021
  registry->RegisterIntegerPref(
      kBraveWalletWeb3ProviderDeprecated,
      static_cast<int>(mojom::DefaultWallet::BraveWalletPreferExtension));

  // Added 25/10/2021
  registry->RegisterIntegerPref(
      kDefaultWalletDeprecated,
      static_cast<int>(mojom::DefaultWallet::BraveWalletPreferExtension));

  // Added 02/2022
  registry->RegisterBooleanPref(
      kBraveWalletEthereumTransactionsCoinTypeMigrated, false);

  // Added 22/02/2022
  registry->RegisterListPref(kBraveWalletCustomNetworksDeprecated);
  registry->RegisterStringPref(kBraveWalletCurrentChainId,
                               brave_wallet::mojom::kMainnetChainId);

  // Added 04/2022
  registry->RegisterDictionaryPref(kBraveWalletUserAssetsDeprecated);

  // Added 06/2022
  registry->RegisterBooleanPref(
      kBraveWalletUserAssetsAddPreloadingNetworksMigrated, false);

  // Added 10/2022
  registry->RegisterBooleanPref(
      kBraveWalletDeprecateEthereumTestNetworksMigrated, false);
}

void ClearJsonRpcServiceProfilePrefs(PrefService* prefs) {
  DCHECK(prefs);
  prefs->ClearPref(kBraveWalletCustomNetworks);
  prefs->ClearPref(kBraveWalletHiddenNetworks);
  prefs->ClearPref(kBraveWalletSelectedNetworks);
  prefs->ClearPref(kSupportEip1559OnLocalhostChain);
}

void ClearKeyringServiceProfilePrefs(PrefService* prefs) {
  DCHECK(prefs);
  prefs->ClearPref(kBraveWalletKeyrings);
  prefs->ClearPref(kBraveWalletAutoLockMinutes);
  prefs->ClearPref(kBraveWalletSelectedAccount);
}

void ClearTxServiceProfilePrefs(PrefService* prefs) {
  DCHECK(prefs);
  prefs->ClearPref(kBraveWalletTransactions);
}

void ClearBraveWalletServicePrefs(PrefService* prefs) {
  DCHECK(prefs);
  prefs->ClearPref(kBraveWalletUserAssets);
  prefs->ClearPref(kDefaultBaseCurrency);
  prefs->ClearPref(kDefaultBaseCryptocurrency);
}

void MigrateObsoleteProfilePrefs(PrefService* prefs) {
  // Added 10/2021 for migrating the contract address for eth in user asset
  // list from 'eth' to an empty string.
  BraveWalletService::MigrateUserAssetEthContractAddress(prefs);

  // Added 04/22 to have coin_type as the top level, also rename
  // contract_address key to address.
  BraveWalletService::MigrateMultichainUserAssets(prefs);

  // Added 06/22 to have native tokens for all preloading networks.
  BraveWalletService::MigrateUserAssetsAddPreloadingNetworks(prefs);

  JsonRpcService::MigrateMultichainNetworks(prefs);

  if (prefs->HasPrefPath(kBraveWalletWeb3ProviderDeprecated)) {
    mojom::DefaultWallet provider = static_cast<mojom::DefaultWallet>(
        prefs->GetInteger(kBraveWalletWeb3ProviderDeprecated));
    mojom::DefaultWallet default_wallet =
        mojom::DefaultWallet::BraveWalletPreferExtension;
    if (provider == mojom::DefaultWallet::None)
      default_wallet = mojom::DefaultWallet::None;
    prefs->SetInteger(kDefaultEthereumWallet, static_cast<int>(default_wallet));
    prefs->ClearPref(kBraveWalletWeb3ProviderDeprecated);
  }
  if (prefs->HasPrefPath(kDefaultWalletDeprecated)) {
    mojom::DefaultWallet provider = static_cast<mojom::DefaultWallet>(
        prefs->GetInteger(kDefaultWalletDeprecated));
    mojom::DefaultWallet default_wallet =
        mojom::DefaultWallet::BraveWalletPreferExtension;
    if (provider == mojom::DefaultWallet::None)
      default_wallet = mojom::DefaultWallet::None;
    prefs->SetInteger(kDefaultEthereumWallet, static_cast<int>(default_wallet));
    prefs->ClearPref(kDefaultWalletDeprecated);
  }

  // Added 02/2022.
  // Migrate kBraveWalletTransactions to have coin_type as the top level.
  // Ethereum transactions were at kBraveWalletTransactions.network_id.tx_id,
  // migrate it to be at kBraveWalletTransactions.ethereum.network_id.tx_id.
  if (!prefs->GetBoolean(kBraveWalletEthereumTransactionsCoinTypeMigrated)) {
    auto transactions = prefs->GetDict(kBraveWalletTransactions).Clone();
    prefs->ClearPref(kBraveWalletTransactions);
    if (!transactions.empty()) {
      DictionaryPrefUpdate update(prefs, kBraveWalletTransactions);
      base::Value* dict = update.Get();
      dict->SetPath(kEthereumPrefKey, base::Value(std::move(transactions)));
    }
    prefs->SetBoolean(kBraveWalletEthereumTransactionsCoinTypeMigrated, true);
  }

  // Added 10/2022
  JsonRpcService::MigrateDeprecatedEthereumTestnets(prefs);
}

}  // namespace brave_wallet
