/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/keyring_service_prefs.h"

#include <string>

#include "base/values.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_wallet {

std::string KeyringIdPrefString(mojom::KeyringId keyring_id) {
  switch (keyring_id) {
    case mojom::KeyringId::kFilecoin:
      return "filecoin";
    case mojom::KeyringId::kFilecoinTestnet:
      return "filecoin_testnet";
    case mojom::KeyringId::kSolana:
      return "solana";
    case mojom::KeyringId::kDefault:
      return "default";
    case mojom::KeyringId::kBitcoin84:
      return "bitcoin_84";
    case mojom::KeyringId::kBitcoin84Testnet:
      return "bitcoin_84_test";
    case mojom::KeyringId::kZCashMainnet:
      return "zcash_mainnet";
    case mojom::KeyringId::kZCashTestnet:
      return "zcash_testnet";
    case mojom::KeyringId::kBitcoinImport:
      return "bitcoin_import";
    case mojom::KeyringId::kBitcoinImportTestnet:
      return "bitcoin_import_test";
  }
  NOTREACHED_NORETURN();
}

const base::Value* GetPrefForKeyring(PrefService* profile_prefs,
                                     const std::string& key,
                                     mojom::KeyringId keyring_id) {
  const auto& keyrings_pref = profile_prefs->GetDict(kBraveWalletKeyrings);
  const base::Value::Dict* keyring_dict =
      keyrings_pref.FindDict(KeyringIdPrefString(keyring_id));
  if (!keyring_dict) {
    return nullptr;
  }

  return keyring_dict->Find(key);
}

void SetPrefForKeyring(PrefService* profile_prefs,
                       const std::string& key,
                       base::Value value,
                       mojom::KeyringId keyring_id) {
  DCHECK(profile_prefs);
  ScopedDictPrefUpdate update(profile_prefs, kBraveWalletKeyrings);
  if (value.is_none()) {
    update->EnsureDict(KeyringIdPrefString(keyring_id))->Remove(key);
  } else {
    update->EnsureDict(KeyringIdPrefString(keyring_id))
        ->Set(key, std::move(value));
  }
}

const base::Value::List* GetPrefForKeyringList(PrefService* profile_prefs,
                                               const std::string& key,
                                               mojom::KeyringId keyring_id) {
  if (const base::Value* result =
          GetPrefForKeyring(profile_prefs, key, keyring_id);
      result && result->is_list()) {
    return result->GetIfList();
  }
  return nullptr;
}

const base::Value::Dict* GetPrefForKeyringDict(PrefService* profile_prefs,
                                               const std::string& key,
                                               mojom::KeyringId keyring_id) {
  if (const base::Value* result =
          GetPrefForKeyring(profile_prefs, key, keyring_id);
      result && result->is_dict()) {
    return result->GetIfDict();
  }
  return nullptr;
}

base::Value::List& GetListPrefForKeyringUpdate(
    ScopedDictPrefUpdate& dict_update,
    const std::string& key,
    mojom::KeyringId keyring_id) {
  return *dict_update.Get()
              .EnsureDict(KeyringIdPrefString(keyring_id))
              ->EnsureList(key);
}

base::Value::Dict& GetDictPrefForKeyringUpdate(
    ScopedDictPrefUpdate& dict_update,
    const std::string& key,
    mojom::KeyringId keyring_id) {
  return *dict_update.Get()
              .EnsureDict(KeyringIdPrefString(keyring_id))
              ->EnsureDict(key);
}

uint32_t GetNextAccountIndex(PrefService* profile_prefs,
                             mojom::KeyringId keyring_id) {
  DCHECK(IsBitcoinImportKeyring(keyring_id));

  if (auto* next_account_index_value =
          GetPrefForKeyring(profile_prefs, kNextAccountIndex, keyring_id)) {
    return next_account_index_value->GetIfInt().value_or(0);
  }
  return 0;
}

void SetNextAccountIndex(PrefService* profile_prefs,
                         mojom::KeyringId keyring_id,
                         uint32_t index) {
  DCHECK(IsBitcoinImportKeyring(keyring_id));

  SetPrefForKeyring(profile_prefs, kNextAccountIndex,
                    base::Value(base::checked_cast<int>(index)), keyring_id);
}

bool SetSelectedWalletAccountInPrefs(PrefService* profile_prefs,
                                     const std::string& unique_key) {
  if (unique_key ==
      profile_prefs->GetString(kBraveWalletSelectedWalletAccount)) {
    return false;
  }

  profile_prefs->SetString(kBraveWalletSelectedWalletAccount, unique_key);
  return true;
}

bool SetSelectedDappAccountInPrefs(PrefService* profile_prefs,
                                   mojom::CoinType dapp_coin,
                                   const std::string& unique_key) {
  CHECK(CoinSupportsDapps(dapp_coin));
  const char* pref_name = dapp_coin == mojom::CoinType::ETH
                              ? kBraveWalletSelectedEthDappAccount
                              : kBraveWalletSelectedSolDappAccount;
  if (unique_key == profile_prefs->GetString(pref_name)) {
    return false;
  }

  profile_prefs->SetString(pref_name, unique_key);
  return true;
}

}  // namespace brave_wallet
