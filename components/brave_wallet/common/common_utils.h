/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_COMMON_UTILS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_COMMON_UTILS_H_

#include <string>
#include <vector>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"

class PrefService;

namespace brave_wallet {

constexpr mojom::CoinType kAllCoins[] = {
    mojom::CoinType::ETH, mojom::CoinType::FIL, mojom::CoinType::SOL,
    mojom::CoinType::BTC};

bool IsNativeWalletEnabled();
bool IsFilecoinEnabled();
bool IsSolanaEnabled();
bool ShouldShowTxStatusInToolbar();
bool IsNftPinningEnabled();
bool IsPanelV2Enabled();
bool ShouldCreateDefaultSolanaAccount();
bool IsDappsSupportEnabled();
bool IsBitcoinEnabled();

bool IsAllowed(PrefService* prefs);

bool IsFilecoinKeyringId(mojom::KeyringId keyring_id);

bool IsBitcoinKeyring(mojom::KeyringId keyring_id);
bool IsBitcoinMainnetKeyring(mojom::KeyringId keyring_id);
bool IsBitcoinTestnetKeyring(mojom::KeyringId keyring_id);
bool IsBitcoinNetwork(const std::string& network_id);
bool IsValidBitcoinNetworkKeyringPair(const std::string& network_id,
                                      mojom::KeyringId keyring_id);
bool IsBitcoinAccount(const mojom::AccountId& account_id);

mojom::KeyringId GetFilecoinKeyringId(const std::string& network);

std::string GetFilecoinChainId(mojom::KeyringId keyring_id);

mojom::CoinType GetCoinForKeyring(mojom::KeyringId keyring_id);

GURL GetActiveEndpointUrl(const mojom::NetworkInfo& chain);

std::vector<mojom::KeyringId> GetSupportedKeyrings();
bool CoinSupportsDapps(mojom::CoinType coin);
std::vector<mojom::KeyringId> GetSupportedKeyringsForNetwork(
    mojom::CoinType coin,
    const std::string& chain_id);

mojom::AccountIdPtr MakeAccountId(mojom::CoinType coin,
                                  mojom::KeyringId keyring_id,
                                  mojom::AccountKind kind,
                                  const std::string& address);
mojom::AccountIdPtr MakeBitcoinAccountId(mojom::CoinType coin,
                                         mojom::KeyringId keyring_id,
                                         mojom::AccountKind kind,
                                         uint32_t account_index);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_COMMON_COMMON_UTILS_H_
