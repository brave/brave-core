/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_COMMON_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_COMMON_UTILS_H_

#include <string>
#include <vector>

#include "base/containers/to_vector.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "url/gurl.h"

class PrefService;

namespace brave_wallet {

bool IsNativeWalletEnabled();
bool IsBitcoinEnabled();
bool IsBitcoinImportEnabled();
bool IsBitcoinLedgerEnabled();
bool IsZCashEnabled();
bool IsCardanoEnabled();
bool IsZCashShieldedTransactionsEnabled();
bool IsAnkrBalancesEnabled();
bool IsTransactionSimulationsEnabled();

bool IsAllowed(PrefService* prefs);

bool IsEthereumKeyring(mojom::KeyringId keyring_id);
bool IsEthereumAccount(const mojom::AccountIdPtr& account_id);

bool IsSolanaKeyring(mojom::KeyringId keyring_id);
bool IsSolanaAccount(const mojom::AccountIdPtr& account_id);

bool IsFilecoinKeyring(mojom::KeyringId keyring_id);
bool IsFilecoinAccount(const mojom::AccountIdPtr& account_id);

bool IsBitcoinKeyring(mojom::KeyringId keyring_id);
bool IsBitcoinMainnetKeyring(mojom::KeyringId keyring_id);
bool IsBitcoinTestnetKeyring(mojom::KeyringId keyring_id);
bool IsBitcoinHDKeyring(mojom::KeyringId keyring_id);
bool IsBitcoinImportKeyring(mojom::KeyringId keyring_id);
bool IsBitcoinHardwareKeyring(mojom::KeyringId keyring_id);
bool IsBitcoinNetwork(const std::string& network_id);
bool IsBitcoinAccount(const mojom::AccountIdPtr& account_id);

bool IsZCashAccount(const mojom::AccountIdPtr& account_id);
bool IsZCashNetwork(const std::string& network_id);
bool IsZCashKeyring(mojom::KeyringId keyring_id);
bool IsZCashMainnetKeyring(mojom::KeyringId keyring_id);
bool IsZCashTestnetKeyring(mojom::KeyringId keyring_id);

bool IsCardanoKeyring(mojom::KeyringId keyring_id);
bool IsCardanoMainnetKeyring(mojom::KeyringId keyring_id);
bool IsCardanoTestnetKeyring(mojom::KeyringId keyring_id);
bool IsCardanoHDKeyring(mojom::KeyringId keyring_id);
bool IsCardanoImportKeyring(mojom::KeyringId keyring_id);
bool IsCardanoHardwareKeyring(mojom::KeyringId keyring_id);
bool IsCardanoNetwork(const std::string& network_id);
bool IsCardanoAccount(const mojom::AccountIdPtr& account_id);

mojom::KeyringId GetFilecoinKeyringId(const std::string& network);

std::string GetFilecoinChainId(mojom::KeyringId keyring_id);

mojom::CoinType GetCoinForKeyring(mojom::KeyringId keyring_id);

mojom::CoinType GetCoinTypeFromTxDataUnion(
    const mojom::TxDataUnion& tx_data_union);

GURL GetActiveEndpointUrl(const mojom::NetworkInfo& chain);

std::vector<mojom::CoinType> GetEnabledCoins();
std::vector<mojom::KeyringId> GetEnabledKeyrings();
bool CoinSupportsDapps(mojom::CoinType coin);
std::vector<mojom::KeyringId> GetSupportedKeyringsForNetwork(
    mojom::CoinType coin,
    const std::string& chain_id);

mojom::AccountIdPtr MakeAccountId(mojom::CoinType coin,
                                  mojom::KeyringId keyring_id,
                                  mojom::AccountKind kind,
                                  const std::string& address);
mojom::AccountIdPtr MakeIndexBasedAccountId(mojom::CoinType coin,
                                            mojom::KeyringId keyring_id,
                                            mojom::AccountKind kind,
                                            uint32_t account_index);
std::string GetNetworkForBitcoinKeyring(const mojom::KeyringId& keyring_id);
std::string GetNetworkForBitcoinAccount(const mojom::AccountIdPtr& account_id);

std::string GetNetworkForZCashKeyring(const mojom::KeyringId& keyring_id);

bool IsHTTPSOrLocalhostURL(const std::string& url);

template <typename T>
std::vector<T> CloneVector(const std::vector<T>& v) {
  return base::ToVector(v, &T::Clone);
}

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_COMMON_UTILS_H_
