/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/common_utils.h"

#include <utility>

#include "base/feature_list.h"
#include "brave/components/brave_wallet/common/features.h"
#include "brave/components/brave_wallet/common/pref_names.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"

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

bool IsFilecoinEnabled() {
  return base::FeatureList::IsEnabled(features::kBraveWalletFilecoinFeature);
}

bool IsDappsSupportEnabled() {
  return base::FeatureList::IsEnabled(
      features::kBraveWalletDappsSupportFeature);
}

bool IsSolanaEnabled() {
  return base::FeatureList::IsEnabled(features::kBraveWalletSolanaFeature);
}

bool IsNftPinningEnabled() {
  return base::FeatureList::IsEnabled(features::kBraveWalletNftPinningFeature);
}

bool IsPanelV2Enabled() {
  return base::FeatureList::IsEnabled(features::kBraveWalletPanelV2Feature);
}

bool ShouldCreateDefaultSolanaAccount() {
  return IsSolanaEnabled() && features::kCreateDefaultSolanaAccount.Get();
}

bool ShouldShowTxStatusInToolbar() {
  return features::kShowToolbarTxStatus.Get();
}

bool IsBitcoinEnabled() {
  return base::FeatureList::IsEnabled(features::kBraveWalletBitcoinFeature);
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
         keyring_id == mojom::KeyringId::kBitcoin84Testnet;
}

bool IsBitcoinMainnetKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kBitcoin84;
}

bool IsBitcoinTestnetKeyring(mojom::KeyringId keyring_id) {
  return keyring_id == mojom::KeyringId::kBitcoin84Testnet;
}

bool IsBitcoinNetwork(const std::string& network_id) {
  return network_id == mojom::kBitcoinMainnet ||
         network_id == mojom::kBitcoinTestnet;
}

bool IsValidBitcoinNetworkKeyringPair(const std::string& network_id,
                                      mojom::KeyringId keyring_id) {
  if (!IsBitcoinKeyring(keyring_id) || !IsBitcoinNetwork(network_id)) {
    return false;
  }

  if (network_id == mojom::kBitcoinMainnet) {
    return IsBitcoinMainnetKeyring(keyring_id);
  } else if (network_id == mojom::kBitcoinTestnet) {
    return IsBitcoinTestnetKeyring(keyring_id);
  }
  NOTREACHED();
  return false;
}

bool IsBitcoinAccount(const mojom::AccountId& account_id) {
  return account_id.coin == mojom::CoinType::BTC &&
         IsBitcoinKeyring(account_id.keyring_id) &&
         account_id.kind == mojom::AccountKind::kDerived;
}

mojom::KeyringId GetFilecoinKeyringId(const std::string& network) {
  if (network == mojom::kFilecoinMainnet) {
    return mojom::KeyringId::kFilecoin;
  } else if (network == mojom::kFilecoinTestnet ||
             network == mojom::kLocalhostChainId) {
    return mojom::KeyringId::kFilecoinTestnet;
  }
  NOTREACHED() << "Unsupported chain id for filecoin " << network;
  return mojom::KeyringId::kFilecoin;
}

std::string GetFilecoinChainId(mojom::KeyringId keyring_id) {
  if (keyring_id == mojom::KeyringId::kFilecoin) {
    return mojom::kFilecoinMainnet;
  } else if (keyring_id == mojom::KeyringId::kFilecoinTestnet) {
    return mojom::kFilecoinTestnet;
  }
  NOTREACHED() << "Unsupported keyring id for filecoin";
  return "";
}

mojom::CoinType GetCoinForKeyring(mojom::KeyringId keyring_id) {
  if (IsFilecoinKeyringId(keyring_id)) {
    return mojom::CoinType::FIL;
  } else if (keyring_id == mojom::KeyringId::kSolana) {
    return mojom::CoinType::SOL;
  } else if (IsBitcoinKeyring(keyring_id)) {
    return mojom::CoinType::BTC;
  }

  DCHECK_EQ(keyring_id, mojom::KeyringId::kDefault);
  return mojom::CoinType::ETH;
}

GURL GetActiveEndpointUrl(const mojom::NetworkInfo& chain) {
  if (chain.active_rpc_endpoint_index >= 0 &&
      static_cast<size_t>(chain.active_rpc_endpoint_index) <
          chain.rpc_endpoints.size()) {
    return chain.rpc_endpoints[chain.active_rpc_endpoint_index];
  }
  return GURL();
}

std::vector<mojom::KeyringId> GetSupportedKeyrings() {
  std::vector<mojom::KeyringId> ids = {mojom::KeyringId::kDefault};
  if (IsFilecoinEnabled()) {
    ids.push_back(mojom::KeyringId::kFilecoin);
    ids.push_back(mojom::KeyringId::kFilecoinTestnet);
  }
  if (IsSolanaEnabled()) {
    ids.push_back(mojom::KeyringId::kSolana);
  }
  if (IsBitcoinEnabled()) {
    ids.push_back(mojom::KeyringId::kBitcoin84);
    ids.push_back(mojom::KeyringId::kBitcoin84Testnet);
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
        return {mojom::KeyringId::kBitcoin84};
      } else {
        return {mojom::KeyringId::kBitcoin84Testnet};
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

  std::string unique_key =
      base::JoinString({base::NumberToString(static_cast<int>(coin)),
                        base::NumberToString(static_cast<int>(keyring_id)),
                        base::NumberToString(static_cast<int>(kind)), address},
                       "_");
  return mojom::AccountId::New(coin, keyring_id, kind, address, 0,
                               std::move(unique_key));
}

mojom::AccountIdPtr MakeBitcoinAccountId(mojom::CoinType coin,
                                         mojom::KeyringId keyring_id,
                                         mojom::AccountKind kind,
                                         uint32_t account_index) {
  DCHECK_EQ(coin, mojom::CoinType::BTC);
  DCHECK(IsBitcoinKeyring(keyring_id));
  DCHECK_EQ(kind, mojom::AccountKind::kDerived);

  std::string unique_key =
      base::JoinString({base::NumberToString(static_cast<int>(coin)),
                        base::NumberToString(static_cast<int>(keyring_id)),
                        base::NumberToString(static_cast<int>(kind)),
                        base::NumberToString(account_index)},
                       "_");
  return mojom::AccountId::New(coin, keyring_id, kind, "", account_index,
                               std::move(unique_key));
}

}  // namespace brave_wallet
