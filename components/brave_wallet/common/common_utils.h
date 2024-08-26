/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_COMMON_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_COMMON_UTILS_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "url/gurl.h"

class PrefService;

namespace brave_wallet {

inline constexpr mojom::CoinType kAllCoins[] = {
    mojom::CoinType::ETH, mojom::CoinType::FIL, mojom::CoinType::SOL,
    mojom::CoinType::BTC, mojom::CoinType::ZEC};

inline constexpr mojom::KeyringId kAllKeyrings[] = {
    mojom::KeyringId::kDefault,
    mojom::KeyringId::kBitcoin84,
    mojom::KeyringId::kBitcoin84Testnet,
    mojom::KeyringId::kFilecoin,
    mojom::KeyringId::kFilecoinTestnet,
    mojom::KeyringId::kSolana,
    mojom::KeyringId::kZCashMainnet,
    mojom::KeyringId::kZCashTestnet,
    mojom::KeyringId::kBitcoinImport,
    mojom::KeyringId::kBitcoinImportTestnet};

bool IsZCashKeyring(mojom::KeyringId keyring_id);

bool IsNativeWalletEnabled();
bool IsBitcoinEnabled();
bool IsBitcoinImportEnabled();
bool IsBitcoinLedgerEnabled();
bool IsZCashEnabled();
bool IsZCashShieldedTransactionsEnabled();
bool IsAnkrBalancesEnabled();
bool IsTransactionSimulationsEnabled();

bool IsAllowed(PrefService* prefs);

bool IsFilecoinKeyringId(mojom::KeyringId keyring_id);

bool IsBitcoinKeyring(mojom::KeyringId keyring_id);
bool IsBitcoinMainnetKeyring(mojom::KeyringId keyring_id);
bool IsBitcoinTestnetKeyring(mojom::KeyringId keyring_id);
bool IsBitcoinHDKeyring(mojom::KeyringId keyring_id);
bool IsBitcoinImportKeyring(mojom::KeyringId keyring_id);
bool IsBitcoinNetwork(const std::string& network_id);
bool IsBitcoinAccount(const mojom::AccountId& account_id);

bool IsZCashAccount(const mojom::AccountId& account_id);
bool IsZCashNetwork(const std::string& network_id);
bool IsZCashKeyring(mojom::KeyringId keyring_id);

mojom::KeyringId GetFilecoinKeyringId(const std::string& network);

std::string GetFilecoinChainId(mojom::KeyringId keyring_id);

mojom::CoinType GetCoinForKeyring(mojom::KeyringId keyring_id);

mojom::CoinType GetCoinTypeFromTxDataUnion(
    const mojom::TxDataUnion& tx_data_union);

GURL GetActiveEndpointUrl(const mojom::NetworkInfo& chain);

std::vector<mojom::CoinType> GetSupportedCoins();
std::vector<mojom::KeyringId> GetSupportedKeyrings();
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

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_COMMON_UTILS_H_
