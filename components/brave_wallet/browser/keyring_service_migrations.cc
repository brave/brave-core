/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/keyring_service_migrations.h"

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/notreached.h"
#include "base/strings/string_number_conversions.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/keyring_service_prefs.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_wallet {

namespace {

std::string GetRootPath(mojom::KeyringId keyring_id) {
  if (keyring_id == mojom::KeyringId::kDefault) {
    return "m/44'/60'/0'/0";
  } else if (keyring_id == mojom::KeyringId::kSolana) {
    return "m/44'/501'";
  } else if (keyring_id == mojom::KeyringId::kFilecoin) {
    return "m/44'/461'/0'/0";
  } else if (keyring_id == mojom::KeyringId::kFilecoinTestnet) {
    return "m/44'/1'/0'/0";
  } else if (keyring_id == mojom::KeyringId::kBitcoin84) {
    return "m/84'/0'";
  } else if (keyring_id == mojom::KeyringId::kBitcoin84Testnet) {
    return "m/84'/1'";
  } else if (keyring_id == mojom::KeyringId::kZCashMainnet) {
    return "m/44'/133'";
  } else if (keyring_id == mojom::KeyringId::kZCashTestnet) {
    return "m/44'/1'";
  }

  NOTREACHED() << keyring_id;
}

std::optional<uint32_t> ExtractAccountIndex(mojom::KeyringId keyring_id,
                                            std::string_view account_index) {
  CHECK(keyring_id == mojom::KeyringId::kDefault ||
        keyring_id == mojom::KeyringId::kFilecoin ||
        keyring_id == mojom::KeyringId::kFilecoinTestnet ||
        keyring_id == mojom::KeyringId::kSolana);

  // m/44'/60'/0'/0/{index}
  // m/44'/461'/0'/0/{index}
  // m/44'/1'/0'/0/{index}
  // m/44'/501'/{index}'/0'

  // For all types remove root path and slash. For Solana also remove '/0'.

  auto root_path = GetRootPath(keyring_id);
  if (!account_index.starts_with(root_path)) {
    return std::nullopt;
  }
  account_index.remove_prefix(root_path.size());

  if (!account_index.starts_with("/")) {
    return std::nullopt;
  }
  account_index.remove_prefix(1);

  if (keyring_id == mojom::KeyringId::kSolana) {
    if (!account_index.ends_with("'/0'")) {
      return std::nullopt;
    }
    account_index.remove_suffix(4);
  }

  uint32_t result = 0;
  if (!base::StringToUint(account_index, &result)) {
    return std::nullopt;
  }

  return result;
}

}  // namespace

// static
void MigrateDerivedAccountIndex(PrefService* profile_prefs) {
  ScopedDictPrefUpdate update(profile_prefs, kBraveWalletKeyrings);

  const std::vector<mojom::KeyringId> keyrings = {
      mojom::KeyringId::kDefault,      mojom::KeyringId::kSolana,
      mojom::KeyringId::kFilecoin,     mojom::KeyringId::kFilecoinTestnet,
      mojom::KeyringId::kBitcoin84,    mojom::KeyringId::kBitcoin84Testnet,
      mojom::KeyringId::kZCashMainnet, mojom::KeyringId::kZCashTestnet};

  for (auto keyring_id : keyrings) {
    base::Value::Dict* keyring_dict =
        update->FindDict(KeyringIdPrefString(keyring_id));
    if (!keyring_dict) {
      continue;
    }

    base::Value::Dict* account_metas_dict =
        keyring_dict->FindDict(kAccountMetas);

    if (!account_metas_dict) {
      continue;
    }

    if (IsBitcoinKeyring(keyring_id)) {
      // Don't bother with migrating bitcoin accounts.
      account_metas_dict->clear();
    }

    std::map<uint32_t, base::Value::Dict> new_accounts_map;
    for (auto acc_item : *account_metas_dict) {
      auto account_index = ExtractAccountIndex(keyring_id, acc_item.first);
      if (!account_index) {
        continue;
      }
      if (!acc_item.second.is_dict()) {
        continue;
      }

      base::Value::Dict new_account = acc_item.second.GetIfDict()->Clone();
      new_account.Set(kAccountIndex, base::NumberToString(*account_index));
      new_accounts_map[*account_index] = std::move(new_account);
    }

    base::Value::List new_accounts;
    for (auto& acc : new_accounts_map) {
      new_accounts.Append(std::move(acc.second));
    }

    keyring_dict->Set(kAccountMetas, std::move(new_accounts));
  }
}

}  // namespace brave_wallet
