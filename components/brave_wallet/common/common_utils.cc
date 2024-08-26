/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/common_utils.h"

#include <utility>

#include "base/feature_list.h"
#include "base/notreached.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/pref_names.h"
#include "brave/net/base/url_util.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"
#include "net/base/url_util.h"

namespace brave_wallet {

namespace {

bool IsDisabledByPolicy(PrefService* prefs) {
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
  DCHECK(prefs);
  return prefs->IsManagedPreference(prefs::kDisabledByPolicy) &&
         prefs->GetBoolean(prefs::kDisabledByPolicy);
#else
  return false;
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
}

}  // namespace

bool IsNativeWalletEnabled() {
  return base::FeatureList::IsEnabled(features::kNativeBraveWalletFeature);
}

bool IsBitcoinEnabled() {
  return base::FeatureList::IsEnabled(features::kBraveWalletBitcoinFeature);
}

bool IsBitcoinImportEnabled() {
  return IsBitcoinEnabled() && base::FeatureList::IsEnabled(
                                   features::kBraveWalletBitcoinImportFeature);
}

bool IsBitcoinLedgerEnabled() {
  return IsBitcoinEnabled() && base::FeatureList::IsEnabled(
                                   features::kBraveWalletBitcoinLedgerFeature);
}

bool IsZCashEnabled() {
  return base::FeatureList::IsEnabled(features::kBraveWalletZCashFeature);
}

bool IsZCashShieldedTransactionsEnabled() {
#if BUILDFLAG(ENABLE_ORCHARD)
  return features::kZCashShieldedTransactionsEnabled.Get();
#else
  return false;
#endif
}

bool IsAnkrBalancesEnabled() {
  return base::FeatureList::IsEnabled(
      features::kBraveWalletAnkrBalancesFeature);
}

bool IsTransactionSimulationsEnabled() {
  return base::FeatureList::IsEnabled(
      features::kBraveWalletTransactionSimulationsFeature);
}

bool IsAllowed(PrefService* prefs) {
  return !IsDisabledByPolicy(prefs);
}

bool IsFilecoinKeyringId(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kFilecoin ||
         keyring_id == mojom::KeyringId::kFilecoinTestnet;
}

bool IsBitcoinKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kBitcoin84 ||
         keyring_id == mojom::KeyringId::kBitcoin84Testnet ||
         keyring_id == mojom::KeyringId::kBitcoinImport ||
         keyring_id == mojom::KeyringId::kBitcoinImportTestnet;
}

bool IsZCashKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kZCashMainnet ||
         keyring_id == mojom::KeyringId::kZCashTestnet;
}

bool IsZCashMainnetKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kZCashMainnet;
}

bool IsZCashTestnetKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kZCashTestnet;
}

bool IsBitcoinMainnetKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kBitcoin84 ||
         keyring_id == mojom::KeyringId::kBitcoinImport;
}

bool IsBitcoinTestnetKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kBitcoin84Testnet ||
         keyring_id == mojom::KeyringId::kBitcoinImportTestnet;
}

bool IsBitcoinHDKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kBitcoin84 ||
         keyring_id == mojom::KeyringId::kBitcoin84Testnet;
}

bool IsBitcoinImportKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kBitcoinImport ||
         keyring_id == mojom::KeyringId::kBitcoinImportTestnet;
}

bool IsBitcoinNetwork(const std::string& network_id) {
  return network_id == mojom::kBitcoinMainnet ||
         network_id == mojom::kBitcoinTestnet;
}

bool IsZCashNetwork(const std::string& network_id) {
  return network_id == mojom::kZCashMainnet ||
         network_id == mojom::kZCashTestnet;
}

bool IsBitcoinAccount(const mojom::AccountId& account_id) {
  return account_id.coin == mojom::CoinType::BTC &&
         IsBitcoinKeyring(account_id.keyring_id);
}

bool IsZCashAccount(const mojom::AccountId& account_id) {
  return account_id.coin == mojom::CoinType::ZEC &&
         IsZCashKeyring(account_id.keyring_id) &&
         account_id.kind == mojom::AccountKind::kDerived;
}

mojom::KeyringId GetFilecoinKeyringId(const std::string& network) {
  if (network == mojom::kFilecoinMainnet) {
    return mojom::KeyringId::kFilecoin;
  } else if (network == mojom::kFilecoinTestnet ||
             network == mojom::kLocalhostChainId) {
    return mojom::KeyringId::kFilecoinTestnet;
  }
  NOTREACHED_IN_MIGRATION() << "Unsupported chain id for filecoin " << network;
  return mojom::KeyringId::kFilecoin;
}

std::string GetFilecoinChainId(mojom::KeyringId keyring_id) {
  if (keyring_id == mojom::KeyringId::kFilecoin) {
    return mojom::kFilecoinMainnet;
  } else if (keyring_id == mojom::KeyringId::kFilecoinTestnet) {
    return mojom::kFilecoinTestnet;
  }
  NOTREACHED_IN_MIGRATION() << "Unsupported keyring id for filecoin";
  return "";
}

mojom::CoinType GetCoinForKeyring(mojom::KeyringId keyring_id) {
  if (IsFilecoinKeyringId(keyring_id)) {
    return mojom::CoinType::FIL;
  } else if (keyring_id == mojom::KeyringId::kSolana) {
    return mojom::CoinType::SOL;
  } else if (IsBitcoinKeyring(keyring_id)) {
    return mojom::CoinType::BTC;
  } else if (IsZCashKeyring(keyring_id)) {
    return mojom::CoinType::ZEC;
  }

  DCHECK_EQ(keyring_id, mojom::KeyringId::kDefault);
  return mojom::CoinType::ETH;
}

mojom::CoinType GetCoinTypeFromTxDataUnion(
    const mojom::TxDataUnion& tx_data_union) {
  if (tx_data_union.is_eth_tx_data_1559() || tx_data_union.is_eth_tx_data()) {
    return mojom::CoinType::ETH;
  }

  if (tx_data_union.is_solana_tx_data()) {
    return mojom::CoinType::SOL;
  }

  if (tx_data_union.is_fil_tx_data()) {
    return mojom::CoinType::FIL;
  }

  if (tx_data_union.is_btc_tx_data()) {
    return mojom::CoinType::BTC;
  }

  if (tx_data_union.is_zec_tx_data()) {
    return mojom::CoinType::ZEC;
  }

  NOTREACHED_NORETURN();
}

GURL GetActiveEndpointUrl(const mojom::NetworkInfo& chain) {
  if (chain.active_rpc_endpoint_index >= 0 &&
      static_cast<size_t>(chain.active_rpc_endpoint_index) <
          chain.rpc_endpoints.size()) {
    return chain.rpc_endpoints[chain.active_rpc_endpoint_index];
  }
  return GURL();
}

std::vector<mojom::CoinType> GetSupportedCoins() {
  std::vector<mojom::CoinType> coins = {
      mojom::CoinType::ETH, mojom::CoinType::SOL, mojom::CoinType::FIL};
  if (IsBitcoinEnabled()) {
    coins.push_back(mojom::CoinType::BTC);
  }
  if (IsZCashEnabled()) {
    coins.push_back(mojom::CoinType::ZEC);
  }
  return coins;
}

std::vector<mojom::KeyringId> GetSupportedKeyrings() {
  std::vector<mojom::KeyringId> ids = {mojom::KeyringId::kDefault};
  ids.push_back(mojom::KeyringId::kFilecoin);
  ids.push_back(mojom::KeyringId::kFilecoinTestnet);
  ids.push_back(mojom::KeyringId::kSolana);
  if (IsBitcoinEnabled()) {
    ids.push_back(mojom::KeyringId::kBitcoin84);
    ids.push_back(mojom::KeyringId::kBitcoin84Testnet);
    if (IsBitcoinImportEnabled()) {
      ids.push_back(mojom::KeyringId::kBitcoinImport);
      ids.push_back(mojom::KeyringId::kBitcoinImportTestnet);
    }
  }
  if (IsZCashEnabled()) {
    ids.push_back(mojom::KeyringId::kZCashMainnet);
    ids.push_back(mojom::KeyringId::kZCashTestnet);
  }

  DCHECK_GT(ids.size(), 0u);
  return ids;
}

bool CoinSupportsDapps(mojom::CoinType coin) {
  return coin == mojom::CoinType::ETH || coin == mojom::CoinType::SOL;
}

std::vector<mojom::KeyringId> GetSupportedKeyringsForNetwork(
    mojom::CoinType coin,
    const std::string& chain_id) {
  switch (coin) {
    case mojom::CoinType::ETH:
      return {mojom::KeyringId::kDefault};
    case mojom::CoinType::SOL:
      return {mojom::KeyringId::kSolana};
    case mojom::CoinType::FIL: {
      if (chain_id == mojom::kFilecoinMainnet) {
        return {mojom::KeyringId::kFilecoin};
      } else {
        return {mojom::KeyringId::kFilecoinTestnet};
      }
    }
    case mojom::CoinType::BTC:
      if (chain_id == mojom::kBitcoinMainnet) {
        return {mojom::KeyringId::kBitcoin84, mojom::KeyringId::kBitcoinImport};
      } else {
        return {mojom::KeyringId::kBitcoin84Testnet,
                mojom::KeyringId::kBitcoinImportTestnet};
      }
    case mojom::CoinType::ZEC:
      if (chain_id == mojom::kZCashMainnet) {
        return {mojom::KeyringId::kZCashMainnet};
      } else {
        return {mojom::KeyringId::kZCashTestnet};
      }
    default:
      NOTREACHED_NORETURN();
  }
}

mojom::AccountIdPtr MakeAccountId(mojom::CoinType coin,
                                  mojom::KeyringId keyring_id,
                                  mojom::AccountKind kind,
                                  const std::string& address) {
  DCHECK_NE(coin, mojom::CoinType::BTC);
  DCHECK_NE(coin, mojom::CoinType::ZEC);
  DCHECK(!IsBitcoinKeyring(keyring_id));
  DCHECK(!IsZCashKeyring(keyring_id));

  std::string unique_key =
      base::JoinString({base::NumberToString(static_cast<int>(coin)),
                        base::NumberToString(static_cast<int>(keyring_id)),
                        base::NumberToString(static_cast<int>(kind)), address},
                       "_");
  return mojom::AccountId::New(coin, keyring_id, kind, address, 0,
                               std::move(unique_key));
}

mojom::AccountIdPtr MakeIndexBasedAccountId(mojom::CoinType coin,
                                            mojom::KeyringId keyring_id,
                                            mojom::AccountKind kind,
                                            uint32_t account_index) {
  DCHECK(coin == mojom::CoinType::BTC || coin == mojom::CoinType::ZEC);
  DCHECK(IsBitcoinKeyring(keyring_id) || IsZCashKeyring(keyring_id));
  DCHECK(
      (coin == mojom::CoinType::BTC &&
       (kind == mojom::AccountKind::kDerived ||
        kind == mojom::AccountKind::kImported)) ||
      (coin == mojom::CoinType::ZEC && (kind == mojom::AccountKind::kDerived)));

  std::string unique_key =
      base::JoinString({base::NumberToString(static_cast<int>(coin)),
                        base::NumberToString(static_cast<int>(keyring_id)),
                        base::NumberToString(static_cast<int>(kind)),
                        base::NumberToString(account_index)},
                       "_");
  return mojom::AccountId::New(coin, keyring_id, kind, "", account_index,
                               std::move(unique_key));
}

std::string GetNetworkForBitcoinKeyring(const mojom::KeyringId& keyring_id) {
  if (IsBitcoinMainnetKeyring(keyring_id)) {
    return mojom::kBitcoinMainnet;
  }
  if (IsBitcoinTestnetKeyring(keyring_id)) {
    return mojom::kBitcoinTestnet;
  }
  NOTREACHED_NORETURN();
}

std::string GetNetworkForBitcoinAccount(const mojom::AccountIdPtr& account_id) {
  CHECK(account_id);
  CHECK(IsBitcoinAccount(*account_id));
  return GetNetworkForBitcoinKeyring(account_id->keyring_id);
}

std::string GetNetworkForZCashKeyring(const mojom::KeyringId& keyring_id) {
  if (IsZCashMainnetKeyring(keyring_id)) {
    return mojom::kZCashMainnet;
  }
  if (IsZCashTestnetKeyring(keyring_id)) {
    return mojom::kZCashTestnet;
  }
  NOTREACHED_NORETURN();
}

bool IsHTTPSOrLocalhostURL(const std::string& url_string) {
  return net::IsHTTPSOrLocalhostURL(GURL(url_string));
}

}  // namespace brave_wallet
