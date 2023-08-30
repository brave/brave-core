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
  return user_assets_pref;
}

base::Value::Dict GetDefaultSelectedNetworks() {
  base::Value::Dict selected_networks;
  selected_networks.Set(kEthereumPrefKey, mojom::kMainnetChainId);
  selected_networks.Set(kSolanaPrefKey, mojom::kSolanaMainnet);
  selected_networks.Set(kFilecoinPrefKey, mojom::kFilecoinMainnet);
  selected_networks.Set(kBitcoinPrefKey, mojom::kBitcoinMainnet);

  return selected_networks;
}

base::Value::Dict GetDefaultSelectedNetworksPerOrigin() {
  base::Value::Dict selected_networks;
  selected_networks.Set(kEthereumPrefKey, base::Value::Dict());
  selected_networks.Set(kSolanaPrefKey, base::Value::Dict());
  selected_networks.Set(kFilecoinPrefKey, base::Value::Dict());
  selected_networks.Set(kBitcoinPrefKey, base::Value::Dict());

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

  // TODO(apaymyshev): fix by
  // https://github.com/brave/brave-browser/issues/31662
  /*
  base::Value::List btc_hidden;
  btc_hidden.Append(mojom::kBitcoinTestnet);
  hidden_networks.Set(kBitcoinPrefKey, std::move(btc_hidden));
  */

  return hidden_networks;
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
  registry->RegisterTimePref(kBraveWalletP3ALastReportTime, base::Time());
  registry->RegisterTimePref(kBraveWalletP3AFirstReportTime, base::Time());
  registry->RegisterListPref(kBraveWalletP3AWeeklyStorage);
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

  // Added 10/2022
  registry->RegisterBooleanPref(kBraveWalletUserAssetsAddIsNFTMigrated, false);

  // Added 11/2022
  p3a_utils::RegisterFeatureUsagePrefs(
      registry, kBraveWalletP3AFirstUnlockTime, kBraveWalletP3ALastUnlockTime,
      kBraveWalletP3AUsedSecondDay, nullptr, nullptr);
  registry->RegisterTimePref(kBraveWalletLastUnlockTime, base::Time());
  registry->RegisterTimePref(kBraveWalletP3ALastReportTime, base::Time());
  registry->RegisterTimePref(kBraveWalletP3AFirstReportTime, base::Time());
  registry->RegisterListPref(kBraveWalletP3AWeeklyStorage);

  // Added 12/2022
  registry->RegisterBooleanPref(kShowWalletTestNetworksDeprecated, false);

  // Added 02/2023
  registry->RegisterBooleanPref(kBraveWalletTransactionsChainIdMigrated, false);

  // Added 03/2023
  registry->RegisterIntegerPref(kBraveWalletDefaultHiddenNetworksVersion, 0);

  // Added 03/2023
  registry->RegisterBooleanPref(kBraveWalletUserAssetsAddIsERC1155Migrated,
                                false);

  // Added 04/2023
  registry->RegisterBooleanPref(kBraveWalletSolanaTransactionsV0SupportMigrated,
                                false);

  // Added 06/2023
  registry->RegisterIntegerPref(
      kBraveWalletSelectedCoinDeprecated,
      static_cast<int>(brave_wallet::mojom::CoinType::ETH));

  // Added 07/2023
  registry->RegisterBooleanPref(kBraveWalletUserAssetsAddIsSpamMigrated, false);

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
  // Added 10/2021 for migrating the contract address for eth in user asset
  // list from 'eth' to an empty string.
  BraveWalletService::MigrateUserAssetEthContractAddress(prefs);

  // Added 04/22 to have coin_type as the top level, also rename
  // contract_address key to address.
  BraveWalletService::MigrateMultichainUserAssets(prefs);

  // Added 06/22 to have native tokens for all preloading networks.
  BraveWalletService::MigrateUserAssetsAddPreloadingNetworks(prefs);

  // Added 10/22 to have is_nft set for existing ERC721 tokens.
  BraveWalletService::MigrateUserAssetsAddIsNFT(prefs);

  // Added 03/23 to add filecoin evm support.
  BraveWalletService::MigrateHiddenNetworks(prefs);

  // Added 03/23 to have is_erc1155 set false for existing ERC1155 tokens.
  BraveWalletService::MigrateUserAssetsAddIsERC1155(prefs);

  // Added 07/23 to have is_spam set false for existing tokens.
  BraveWalletService::MigrateUserAssetsAddIsSpam(prefs);

  // Added 08/09 to add Fantom as a custom network if selected for the default
  // or custom origins.
  BraveWalletService::MigrateFantomMainnetAsCustomNetwork(prefs);

  JsonRpcService::MigrateMultichainNetworks(prefs);

  if (prefs->HasPrefPath(kBraveWalletWeb3ProviderDeprecated)) {
    mojom::DefaultWallet provider = static_cast<mojom::DefaultWallet>(
        prefs->GetInteger(kBraveWalletWeb3ProviderDeprecated));
    mojom::DefaultWallet default_wallet =
        mojom::DefaultWallet::BraveWalletPreferExtension;
    if (provider == mojom::DefaultWallet::None) {
      default_wallet = mojom::DefaultWallet::None;
    }
    prefs->SetInteger(kDefaultEthereumWallet, static_cast<int>(default_wallet));
    prefs->ClearPref(kBraveWalletWeb3ProviderDeprecated);
  }
  if (prefs->HasPrefPath(kDefaultWalletDeprecated)) {
    mojom::DefaultWallet provider = static_cast<mojom::DefaultWallet>(
        prefs->GetInteger(kDefaultWalletDeprecated));
    mojom::DefaultWallet default_wallet =
        mojom::DefaultWallet::BraveWalletPreferExtension;
    if (provider == mojom::DefaultWallet::None) {
      default_wallet = mojom::DefaultWallet::None;
    }
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
      ScopedDictPrefUpdate update(prefs, kBraveWalletTransactions);
      update->Set(kEthereumPrefKey, std::move(transactions));
    }
    prefs->SetBoolean(kBraveWalletEthereumTransactionsCoinTypeMigrated, true);
  }
  // Added 10/2022
  JsonRpcService::MigrateDeprecatedEthereumTestnets(prefs);

  // Added 12/2022
  JsonRpcService::MigrateShowTestNetworksToggle(prefs);

  // Added 02/2023
  TxStateManager::MigrateAddChainIdToTransactionInfo(prefs);

  // Added 07/2023
  KeyringService::MigrateDerivedAccountIndex(prefs);
}

}  // namespace brave_wallet
