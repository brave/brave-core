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
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "components/sync_preferences/pref_service_syncable.h"

namespace {

base::Value GetDefaultUserAssets() {
  base::Value eth(base::Value::Type::DICTIONARY);
  eth.SetKey("contract_address", base::Value(""));
  eth.SetKey("name", base::Value("Ethereum"));
  eth.SetKey("symbol", base::Value("ETH"));
  eth.SetKey("is_erc20", base::Value(false));
  eth.SetKey("is_erc721", base::Value(false));
  eth.SetKey("decimals", base::Value(18));
  eth.SetKey("visible", base::Value(true));

  base::Value bat(base::Value::Type::DICTIONARY);
  bat.SetKey("contract_address",
             base::Value("0x0D8775F648430679A709E98d2b0Cb6250d2887EF"));
  bat.SetKey("name", base::Value("Basic Attention Token"));
  bat.SetKey("symbol", base::Value("BAT"));
  bat.SetKey("is_erc20", base::Value(true));
  bat.SetKey("is_erc721", base::Value(false));
  bat.SetKey("decimals", base::Value(18));
  bat.SetKey("visible", base::Value(true));
  bat.SetKey("logo", base::Value("bat.png"));

  // Show ETH and BAT by default for mainnet, and ETH for other known networks.
  base::Value user_assets_pref(base::Value::Type::DICTIONARY);

  std::vector<std::string> network_ids =
      brave_wallet::GetAllKnownEthNetworkIds();
  for (const auto& network_id : network_ids) {
    base::Value* user_assets_list = user_assets_pref.SetKey(
        network_id, base::Value(base::Value::Type::LIST));
    user_assets_list->Append(eth.Clone());
    if (network_id == "mainnet")
      user_assets_list->Append(bat.Clone());
  }

  return user_assets_pref;
}

base::Value GetDefaultSelectedNetworks() {
  base::Value selected_networks(base::Value::Type::DICTIONARY);
  selected_networks.SetStringKey(brave_wallet::kEthereumPrefKey,
                                 brave_wallet::mojom::kMainnetChainId);
  selected_networks.SetStringKey(brave_wallet::kSolanaPrefKey,
                                 brave_wallet::mojom::kSolanaMainnet);
  selected_networks.SetStringKey(brave_wallet::kFilecoinPrefKey,
                                 brave_wallet::mojom::kFilecoinMainnet);

  return selected_networks;
}

}  // namespace

namespace brave_wallet {

void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterIntegerPref(
      kDefaultWallet2,
      static_cast<int>(
          brave_wallet::mojom::DefaultWallet::BraveWalletPreferExtension));
  registry->RegisterStringPref(kDefaultBaseCurrency, "USD");
  registry->RegisterStringPref(kDefaultBaseCryptocurrency, "BTC");
  registry->RegisterBooleanPref(kShowWalletIconOnToolbar, true);
  registry->RegisterDictionaryPref(kBraveWalletTransactions);
  registry->RegisterTimePref(kBraveWalletLastUnlockTime, base::Time());
  registry->RegisterTimePref(kBraveWalletP3ALastReportTime, base::Time());
  registry->RegisterTimePref(kBraveWalletP3AFirstReportTime, base::Time());
  registry->RegisterListPref(kBraveWalletP3AWeeklyStorage);
  registry->RegisterDictionaryPref(kBraveWalletKeyrings);
  registry->RegisterDictionaryPref(kBraveWalletCustomNetworks);
  registry->RegisterDictionaryPref(kBraveWalletSelectedNetworks,
                                   GetDefaultSelectedNetworks());
  registry->RegisterDictionaryPref(kBraveWalletUserAssets,
                                   GetDefaultUserAssets());
  registry->RegisterIntegerPref(kBraveWalletAutoLockMinutes, 5);
  registry->RegisterStringPref(kBraveWalletSelectedAccount, "");
  registry->RegisterBooleanPref(kSupportEip1559OnLocalhostChain, false);
}

void RegisterProfilePrefsForMigration(
    user_prefs::PrefRegistrySyncable* registry) {
  // Added 08/2021
  registry->RegisterStringPref(kBraveWalletPasswordEncryptorSalt, "");
  registry->RegisterStringPref(kBraveWalletPasswordEncryptorNonce, "");
  registry->RegisterStringPref(kBraveWalletEncryptedMnemonic, "");
  registry->RegisterIntegerPref(kBraveWalletDefaultKeyringAccountNum, 0);
  registry->RegisterBooleanPref(kBraveWalletBackupComplete, false);
  registry->RegisterListPref(kBraveWalletAccountNames);

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
}

void ClearJsonRpcServiceProfilePrefs(PrefService* prefs) {
  DCHECK(prefs);
  prefs->ClearPref(kBraveWalletCustomNetworks);
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
  JsonRpcService::MigrateMultichainNetworks(prefs);

  if (prefs->HasPrefPath(kBraveWalletWeb3ProviderDeprecated)) {
    mojom::DefaultWallet provider = static_cast<mojom::DefaultWallet>(
        prefs->GetInteger(kBraveWalletWeb3ProviderDeprecated));
    mojom::DefaultWallet default_wallet =
        mojom::DefaultWallet::BraveWalletPreferExtension;
    if (provider == mojom::DefaultWallet::None)
      default_wallet = mojom::DefaultWallet::None;
    prefs->SetInteger(kDefaultWallet2, static_cast<int>(default_wallet));
    prefs->ClearPref(kBraveWalletWeb3ProviderDeprecated);
  }
  if (prefs->HasPrefPath(kDefaultWalletDeprecated)) {
    mojom::DefaultWallet provider = static_cast<mojom::DefaultWallet>(
        prefs->GetInteger(kDefaultWalletDeprecated));
    mojom::DefaultWallet default_wallet =
        mojom::DefaultWallet::BraveWalletPreferExtension;
    if (provider == mojom::DefaultWallet::None)
      default_wallet = mojom::DefaultWallet::None;
    prefs->SetInteger(kDefaultWallet2, static_cast<int>(default_wallet));
    prefs->ClearPref(kDefaultWalletDeprecated);
  }

  // Added 02/2022.
  // Migrate kBraveWalletTransactions to have coin_type as the top level.
  // Ethereum transactions were at kBraveWalletTransactions.network_id.tx_id,
  // migrate it to be at kBraveWalletTransactions.ethereum.network_id.tx_id.
  if (!prefs->GetBoolean(kBraveWalletEthereumTransactionsCoinTypeMigrated)) {
    base::Value transactions =
        prefs->GetDictionary(kBraveWalletTransactions)->Clone();
    prefs->ClearPref(kBraveWalletTransactions);
    if (!transactions.DictEmpty()) {
      DictionaryPrefUpdate update(prefs, kBraveWalletTransactions);
      base::Value* dict = update.Get();
      dict->SetPath(kEthereumPrefKey, std::move(transactions));
    }
    prefs->SetBoolean(kBraveWalletEthereumTransactionsCoinTypeMigrated, true);
  }
}

}  // namespace brave_wallet
