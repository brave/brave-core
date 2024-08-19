/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_SERVICE_PREFS_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_SERVICE_PREFS_H_

#include <string>

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"  // IWYU pragma: keep
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_wallet {

inline constexpr char kAccountMetas[] = "account_metas";
inline constexpr char kAccountName[] = "account_name";
inline constexpr char kAccountIndex[] = "account_index";
inline constexpr char kHardwareVendor[] = "hardware_vendor";
inline constexpr char kImportedAccounts[] = "imported_accounts";
inline constexpr char kAccountAddress[] = "account_address";
inline constexpr char kEncryptedPrivateKey[] = "encrypted_private_key";
inline constexpr char kCoinType[] = "coin_type";
inline constexpr char kHardwareAccounts[] = "hardware";
inline constexpr char kHardwareDerivationPath[] = "derivation_path";
inline constexpr char kBitcoinNextReceiveIndex[] = "bitcoin.next_receive";
inline constexpr char kBitcoinNextChangeIndex[] = "bitcoin.next_change";
inline constexpr char kNextAccountIndex[] = "next_account_index";
inline constexpr char kZcashAccountBirthdayBlockId[] =
    "zcash.account_birthday.block_id";
inline constexpr char kZcashAccountBirthdayBlockHash[] =
    "zcash.account_birthday.block_hash";

std::string KeyringIdPrefString(mojom::KeyringId keyring_id);

// Gets a `key`ed value for a given keyring from prefs.
const base::Value* GetPrefForKeyring(PrefService* profile_prefs,
                                     const std::string& key,
                                     mojom::KeyringId keyring_id);
// Sets a `key`ed `value` for a given keyring to prefs. Clears `key` if
// `value` is none.
void SetPrefForKeyring(PrefService* profile_prefs,
                       const std::string& key,
                       base::Value value,
                       mojom::KeyringId keyring_id);

const base::Value::List* GetPrefForKeyringList(PrefService* profile_prefs,
                                               const std::string& key,
                                               mojom::KeyringId keyring_id);
const base::Value::Dict* GetPrefForKeyringDict(PrefService* profile_prefs,
                                               const std::string& key,
                                               mojom::KeyringId keyring_id);
base::Value::List& GetListPrefForKeyringUpdate(
    ScopedDictPrefUpdate& dict_update,
    const std::string& key,
    mojom::KeyringId keyring_id);
base::Value::Dict& GetDictPrefForKeyringUpdate(
    ScopedDictPrefUpdate& dict_update,
    const std::string& key,
    mojom::KeyringId keyring_id);

uint32_t GetNextAccountIndex(PrefService* profile_prefs,
                             mojom::KeyringId keyring_id);
void SetNextAccountIndex(PrefService* profile_prefs,
                         mojom::KeyringId keyring_id,
                         uint32_t index);

bool SetSelectedWalletAccountInPrefs(PrefService* profile_prefs,
                                     const std::string& unique_key);
bool SetSelectedDappAccountInPrefs(PrefService* profile_prefs,
                                   mojom::CoinType dapp_coin,
                                   const std::string& unique_key);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_SERVICE_PREFS_H_
