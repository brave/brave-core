/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/common_utils.h"

#include <utility>

#include "base/feature_list.h"
#include "base/notreached.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
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

bool IsCardanoEnabled() {
  return base::FeatureList::IsEnabled(features::kBraveWalletCardanoFeature);
}

bool IsZCashEnabled() {
  return base::FeatureList::IsEnabled(features::kBraveWalletZCashFeature);
}

bool IsZCashShieldedTransactionsEnabled() {
#if BUILDFLAG(ENABLE_ORCHARD)
  return IsZCashEnabled() && features::kZCashShieldedTransactionsEnabled.Get();
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

bool IsEthereumKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::kDefaultKeyringId;
}

bool IsEthereumAccount(const mojom::AccountIdPtr& account_id) {
  return account_id && account_id->coin == mojom::CoinType::ETH &&
         IsEthereumKeyring(account_id->keyring_id);
}

bool IsSolanaKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::kSolanaKeyringId;
}

bool IsSolanaAccount(const mojom::AccountIdPtr& account_id) {
  return account_id && account_id->coin == mojom::CoinType::SOL &&
         IsSolanaKeyring(account_id->keyring_id);
}

bool IsFilecoinKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kFilecoin ||
         keyring_id == mojom::KeyringId::kFilecoinTestnet;
}

bool IsFilecoinAccount(const mojom::AccountIdPtr& account_id) {
  return account_id && account_id->coin == mojom::CoinType::FIL &&
         IsFilecoinKeyring(account_id->keyring_id);
}

mojom::KeyringId GetFilecoinKeyringId(const std::string& network) {
  if (network == mojom::kFilecoinMainnet) {
    return mojom::KeyringId::kFilecoin;
  } else if (network == mojom::kFilecoinTestnet ||
             network == mojom::kLocalhostChainId) {
    return mojom::KeyringId::kFilecoinTestnet;
  }
  NOTREACHED() << "Unsupported chain id for filecoin " << network;
}

std::string GetFilecoinChainId(mojom::KeyringId keyring_id) {
  if (keyring_id == mojom::KeyringId::kFilecoin) {
    return mojom::kFilecoinMainnet;
  } else if (keyring_id == mojom::KeyringId::kFilecoinTestnet) {
    return mojom::kFilecoinTestnet;
  }
  NOTREACHED() << "Unsupported keyring id for filecoin";
}

bool IsBitcoinKeyring(mojom::KeyringId keyring_id) {
  return IsBitcoinHDKeyring(keyring_id) || IsBitcoinImportKeyring(keyring_id) ||
         IsBitcoinHardwareKeyring(keyring_id);
}

bool IsBitcoinMainnetKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kBitcoin84 ||
         keyring_id == mojom::KeyringId::kBitcoinImport ||
         keyring_id == mojom::KeyringId::kBitcoinHardware;
}

bool IsBitcoinTestnetKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kBitcoin84Testnet ||
         keyring_id == mojom::KeyringId::kBitcoinImportTestnet ||
         keyring_id == mojom::KeyringId::kBitcoinHardwareTestnet;
}

bool IsBitcoinHDKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kBitcoin84 ||
         keyring_id == mojom::KeyringId::kBitcoin84Testnet;
}

bool IsBitcoinImportKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kBitcoinImport ||
         keyring_id == mojom::KeyringId::kBitcoinImportTestnet;
}

bool IsBitcoinHardwareKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kBitcoinHardware ||
         keyring_id == mojom::KeyringId::kBitcoinHardwareTestnet;
}

bool IsBitcoinNetwork(const std::string& network_id) {
  return network_id == mojom::kBitcoinMainnet ||
         network_id == mojom::kBitcoinTestnet;
}

bool IsBitcoinAccount(const mojom::AccountIdPtr& account_id) {
  return account_id && account_id->coin == mojom::CoinType::BTC &&
         IsBitcoinKeyring(account_id->keyring_id);
}

std::string GetNetworkForBitcoinKeyring(const mojom::KeyringId& keyring_id) {
  if (IsBitcoinMainnetKeyring(keyring_id)) {
    return mojom::kBitcoinMainnet;
  }
  if (IsBitcoinTestnetKeyring(keyring_id)) {
    return mojom::kBitcoinTestnet;
  }
  NOTREACHED();
}

std::string GetNetworkForBitcoinAccount(const mojom::AccountIdPtr& account_id) {
  CHECK(IsBitcoinAccount(account_id));
  return GetNetworkForBitcoinKeyring(account_id->keyring_id);
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

bool IsZCashNetwork(const std::string& network_id) {
  return network_id == mojom::kZCashMainnet ||
         network_id == mojom::kZCashTestnet;
}

bool IsZCashAccount(const mojom::AccountIdPtr& account_id) {
  return account_id && account_id->coin == mojom::CoinType::ZEC &&
         IsZCashKeyring(account_id->keyring_id) &&
         account_id->kind == mojom::AccountKind::kDerived;
}

std::string GetNetworkForZCashKeyring(const mojom::KeyringId& keyring_id) {
  if (IsZCashMainnetKeyring(keyring_id)) {
    return mojom::kZCashMainnet;
  }
  if (IsZCashTestnetKeyring(keyring_id)) {
    return mojom::kZCashTestnet;
  }
  NOTREACHED();
}

bool IsCardanoKeyring(mojom::KeyringId keyring_id) {
  return IsCardanoHDKeyring(keyring_id) || IsCardanoImportKeyring(keyring_id) ||
         IsCardanoHardwareKeyring(keyring_id);
}

bool IsCardanoMainnetKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kCardanoMainnet;
}

bool IsCardanoTestnetKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kCardanoTestnet;
}

bool IsCardanoHDKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kCardanoMainnet ||
         keyring_id == mojom::KeyringId::kCardanoTestnet;
}

bool IsCardanoImportKeyring(mojom::KeyringId keyring_id) {
  return false;  // Not supported yet.
}

bool IsCardanoHardwareKeyring(mojom::KeyringId keyring_id) {
  return false;  // Not supported yet.
}

bool IsCardanoNetwork(const std::string& network_id) {
  return network_id == mojom::kCardanoMainnet ||
         network_id == mojom::kCardanoTestnet;
}

bool IsCardanoAccount(const mojom::AccountIdPtr& account_id) {
  return account_id && account_id->coin == mojom::CoinType::ADA &&
         IsCardanoKeyring(account_id->keyring_id);
}

std::string GetNetworkForCardanoKeyring(const mojom::KeyringId& keyring_id) {
  if (IsCardanoMainnetKeyring(keyring_id)) {
    return mojom::kCardanoMainnet;
  }
  if (IsCardanoTestnetKeyring(keyring_id)) {
    return mojom::kCardanoTestnet;
  }
  NOTREACHED();
}

std::string GetNetworkForCardanoAccount(const mojom::AccountIdPtr& account_id) {
  CHECK(IsCardanoAccount(account_id));
  return GetNetworkForCardanoKeyring(account_id->keyring_id);
}

mojom::CoinType GetCoinForKeyring(mojom::KeyringId keyring_id) {
  if (IsEthereumKeyring(keyring_id)) {
    return mojom::CoinType::ETH;
  }

  if (IsSolanaKeyring(keyring_id)) {
    return mojom::CoinType::SOL;
  }

  if (IsFilecoinKeyring(keyring_id)) {
    return mojom::CoinType::FIL;
  }

  if (IsBitcoinKeyring(keyring_id)) {
    return mojom::CoinType::BTC;
  }

  if (IsZCashKeyring(keyring_id)) {
    return mojom::CoinType::ZEC;
  }

  if (IsCardanoKeyring(keyring_id)) {
    return mojom::CoinType::ADA;
  }

  NOTREACHED() << "Unknown keyring: " << keyring_id;
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

  NOTREACHED();
}

GURL GetActiveEndpointUrl(const mojom::NetworkInfo& chain) {
  if (chain.active_rpc_endpoint_index >= 0 &&
      static_cast<size_t>(chain.active_rpc_endpoint_index) <
          chain.rpc_endpoints.size()) {
    return chain.rpc_endpoints[chain.active_rpc_endpoint_index];
  }
  return GURL();
}

std::vector<mojom::CoinType> GetEnabledCoins() {
  std::vector<mojom::CoinType> coins = {
      mojom::CoinType::ETH, mojom::CoinType::SOL, mojom::CoinType::FIL};

  if (IsBitcoinEnabled()) {
    coins.push_back(mojom::CoinType::BTC);
  }
  if (IsZCashEnabled()) {
    coins.push_back(mojom::CoinType::ZEC);
  }
  if (IsCardanoEnabled()) {
    coins.push_back(mojom::CoinType::ADA);
  }
  return coins;
}

std::vector<mojom::KeyringId> GetEnabledKeyrings() {
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
    if (IsBitcoinLedgerEnabled()) {
      ids.push_back(mojom::KeyringId::kBitcoinHardware);
      ids.push_back(mojom::KeyringId::kBitcoinHardwareTestnet);
    }
  }
  if (IsZCashEnabled()) {
    ids.push_back(mojom::KeyringId::kZCashMainnet);
    ids.push_back(mojom::KeyringId::kZCashTestnet);
  }
  if (IsCardanoEnabled()) {
    ids.push_back(mojom::KeyringId::kCardanoMainnet);
    ids.push_back(mojom::KeyringId::kCardanoTestnet);
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
        return {mojom::KeyringId::kBitcoin84, mojom::KeyringId::kBitcoinImport,
                mojom::KeyringId::kBitcoinHardware};
      } else {
        return {mojom::KeyringId::kBitcoin84Testnet,
                mojom::KeyringId::kBitcoinImportTestnet,
                mojom::KeyringId::kBitcoinHardwareTestnet};
      }
    case mojom::CoinType::ZEC:
      if (chain_id == mojom::kZCashMainnet) {
        return {mojom::KeyringId::kZCashMainnet};
      } else {
        return {mojom::KeyringId::kZCashTestnet};
      }
    case mojom::CoinType::ADA:
      if (chain_id == mojom::kCardanoMainnet) {
        return {mojom::KeyringId::kCardanoMainnet};
      } else {
        return {mojom::KeyringId::kCardanoTestnet};
      }
  }
  NOTREACHED();
}

mojom::AccountIdPtr MakeAccountId(mojom::CoinType coin,
                                  mojom::KeyringId keyring_id,
                                  mojom::AccountKind kind,
                                  const std::string& address) {
  DCHECK_NE(coin, mojom::CoinType::BTC);
  DCHECK_NE(coin, mojom::CoinType::ZEC);
  DCHECK_NE(coin, mojom::CoinType::ADA);
  DCHECK(!IsBitcoinKeyring(keyring_id));
  DCHECK(!IsZCashKeyring(keyring_id));
  DCHECK(!IsCardanoKeyring(keyring_id));

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
#if DCHECK_IS_ON()
  DCHECK(coin == mojom::CoinType::BTC || coin == mojom::CoinType::ZEC ||
         coin == mojom::CoinType::ADA);
  if (coin == mojom::CoinType::BTC) {
    DCHECK(IsBitcoinKeyring(keyring_id));
    if (IsBitcoinHDKeyring(keyring_id)) {
      DCHECK_EQ(kind, mojom::AccountKind::kDerived);
    }
    if (IsBitcoinImportKeyring(keyring_id)) {
      DCHECK_EQ(kind, mojom::AccountKind::kImported);
    }
    if (IsBitcoinHardwareKeyring(keyring_id)) {
      DCHECK_EQ(kind, mojom::AccountKind::kHardware);
    }
  }
  if (coin == mojom::CoinType::ZEC) {
    DCHECK(IsZCashKeyring(keyring_id));
    DCHECK_EQ(kind, mojom::AccountKind::kDerived);
  }
  if (coin == mojom::CoinType::ADA) {
    DCHECK(IsCardanoKeyring(keyring_id));
    DCHECK_EQ(kind, mojom::AccountKind::kDerived);
  }
#endif

  std::string unique_key =
      base::JoinString({base::NumberToString(static_cast<int>(coin)),
                        base::NumberToString(static_cast<int>(keyring_id)),
                        base::NumberToString(static_cast<int>(kind)),
                        base::NumberToString(account_index)},
                       "_");
  return mojom::AccountId::New(coin, keyring_id, kind, "", account_index,
                               std::move(unique_key));
}

bool IsHTTPSOrLocalhostURL(const std::string& url_string) {
  return net::IsHTTPSOrLocalhostURL(GURL(url_string));
}

}  // namespace brave_wallet
