/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/keyring_service_prefs.h"

#include <string>

#include "base/values.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_wallet {

namespace {
constexpr char kHardwareVendor[] = "hardware.vendor";
constexpr char kHardwareDerivationPath[] = "hardware.derivation_path";
constexpr char kHardwareDeviceId[] = "hardware.device_id";
}  // namespace

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
    case mojom::KeyringId::kBitcoinHardware:
      return "bitcoin_hardware";
    case mojom::KeyringId::kBitcoinHardwareTestnet:
      return "bitcoin_hardware_test";
  }
  NOTREACHED();
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

uint32_t GenerateNextAccountIndex(PrefService* profile_prefs,
                                  mojom::KeyringId keyring_id) {
  DCHECK(IsBitcoinImportKeyring(keyring_id) ||
         IsBitcoinHardwareKeyring(keyring_id));

  uint32_t next_index = 0;
  if (auto* next_account_index_value =
          GetPrefForKeyring(profile_prefs, kNextAccountIndex, keyring_id)) {
    next_index = next_account_index_value->GetIfInt().value_or(0);
  }
  SetPrefForKeyring(profile_prefs, kNextAccountIndex,
                    base::Value(base::checked_cast<int>(next_index + 1)),
                    keyring_id);
  return next_index;
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

HardwareAccountInfo::HardwareAccountInfo() = default;
HardwareAccountInfo::HardwareAccountInfo(mojom::KeyringId keyring_id,
                                         uint32_t account_index,
                                         const std::string& account_name,
                                         mojom::HardwareVendor hardware_vendor,
                                         const std::string& derivation_path,
                                         const std::string& device_id)
    : keyring_id(keyring_id),
      account_index(account_index),
      account_name(account_name),
      hardware_vendor(hardware_vendor),
      derivation_path(derivation_path),
      device_id(device_id) {}
HardwareAccountInfo::HardwareAccountInfo(const HardwareAccountInfo& other) =
    default;
HardwareAccountInfo& HardwareAccountInfo::operator=(
    const HardwareAccountInfo& other) = default;
HardwareAccountInfo::~HardwareAccountInfo() = default;
HardwareAccountInfo::HardwareAccountInfo(HardwareAccountInfo&& other) = default;
HardwareAccountInfo& HardwareAccountInfo::operator=(
    HardwareAccountInfo&& other) = default;
bool HardwareAccountInfo::operator==(const HardwareAccountInfo& other) const {
  return std::tie(this->keyring_id, this->account_index, this->account_name,
                  this->hardware_vendor, this->derivation_path, this->device_id,
                  this->bitcoin_xpub, this->bitcoin_next_receive_address_index,
                  this->bitcoin_next_change_address_index) ==
         std::tie(other.keyring_id, other.account_index, other.account_name,
                  other.hardware_vendor, other.derivation_path, other.device_id,
                  other.bitcoin_xpub, other.bitcoin_next_receive_address_index,
                  other.bitcoin_next_change_address_index);
}

mojom::AccountIdPtr HardwareAccountInfo::GetAccountId() const {
  return MakeIndexBasedAccountId(GetCoinForKeyring(keyring_id), keyring_id,
                                 mojom::AccountKind::kHardware, account_index);
}

mojom::AccountInfoPtr HardwareAccountInfo::MakeAccountInfo() const {
  return mojom::AccountInfo::New(
      GetAccountId(), "", account_name,
      mojom::HardwareInfo::New(derivation_path, hardware_vendor, device_id));
}

base::Value HardwareAccountInfo::ToValue() const {
  // Only BTC is supported by now.
  CHECK(bitcoin_xpub);
  CHECK(bitcoin_next_receive_address_index.has_value());
  CHECK(bitcoin_next_change_address_index.has_value());

  base::Value::Dict hw_account;
  hw_account.Set(kAccountIndex, base::checked_cast<int>(account_index));
  hw_account.Set(kAccountName, account_name);
  hw_account.SetByDottedPath(kHardwareVendor,
                             hardware_vendor == mojom::HardwareVendor::kLedger
                                 ? kLedgerPrefValue
                                 : kTrezorPrefValue);
  hw_account.SetByDottedPath(kHardwareDerivationPath, derivation_path);
  hw_account.SetByDottedPath(kHardwareDeviceId, device_id);
  if (bitcoin_xpub) {
    hw_account.SetByDottedPath(kBitcoinXpub, *bitcoin_xpub);
  }
  if (bitcoin_next_receive_address_index) {
    hw_account.SetByDottedPath(
        kBitcoinNextReceiveIndex,
        base::NumberToString(*bitcoin_next_receive_address_index));
  }
  if (bitcoin_next_change_address_index) {
    hw_account.SetByDottedPath(
        kBitcoinNextChangeIndex,
        base::NumberToString(*bitcoin_next_change_address_index));
  }
  return base::Value(std::move(hw_account));
}

// static
std::optional<HardwareAccountInfo> HardwareAccountInfo::FromValue(
    mojom::KeyringId keyring_id,
    const base::Value& value) {
  if (!value.is_dict()) {
    return std::nullopt;
  }
  auto& value_dict = value.GetDict();

  const std::string* account_name = value_dict.FindString(kAccountName);
  const std::optional<int> account_index = value_dict.FindInt(kAccountIndex);
  const std::string* hardware_vendor =
      value_dict.FindStringByDottedPath(kHardwareVendor);
  const std::string* derivation_path =
      value_dict.FindStringByDottedPath(kHardwareDerivationPath);
  const std::string* device_id =
      value_dict.FindStringByDottedPath(kHardwareDeviceId);
  if (!account_name || !account_index ||
      !base::IsValueInRangeForNumericType<uint32_t>(*account_index) ||
      !hardware_vendor || !derivation_path || !device_id) {
    return std::nullopt;
  }

  HardwareAccountInfo account_info(
      keyring_id, base::checked_cast<uint32_t>(*account_index), *account_name,
      *hardware_vendor == kLedgerPrefValue ? mojom::HardwareVendor::kLedger
                                           : mojom::HardwareVendor::kTrezor,
      *derivation_path, *device_id);

  if (auto* bitcoin_receive_string =
          value_dict.FindStringByDottedPath(kBitcoinNextReceiveIndex)) {
    uint32_t bitcoin_receive_value = 0;
    if (!base::StringToUint(*bitcoin_receive_string, &bitcoin_receive_value)) {
      return std::nullopt;
    }
    account_info.bitcoin_next_receive_address_index = bitcoin_receive_value;
  }
  if (auto* bitcoin_change_string =
          value_dict.FindStringByDottedPath(kBitcoinNextChangeIndex)) {
    uint32_t bitcoin_change_value = 0;
    if (!base::StringToUint(*bitcoin_change_string, &bitcoin_change_value)) {
      return std::nullopt;
    }
    account_info.bitcoin_next_change_address_index = bitcoin_change_value;
  }
  if (const std::string* bitcoin_xpub =
          value_dict.FindStringByDottedPath(kBitcoinXpub)) {
    account_info.bitcoin_xpub = *bitcoin_xpub;
  }

  return account_info;
}

std::vector<HardwareAccountInfo> GetHardwareAccountsForKeyring(
    PrefService* profile_prefs,
    mojom::KeyringId keyring_id) {
  CHECK(IsBitcoinHardwareKeyring(keyring_id));
  const base::Value::List* hw_accounts =
      GetPrefForKeyringList(profile_prefs, kAccountMetas, keyring_id);
  if (!hw_accounts) {
    return {};
  }

  std::vector<HardwareAccountInfo> result;
  for (auto& item : *hw_accounts) {
    if (auto derived_account =
            HardwareAccountInfo::FromValue(keyring_id, item)) {
      result.emplace_back(std::move(*derived_account));
    }
  }

  return result;
}

void SetHardwareAccountsForKeyring(
    PrefService* profile_prefs,
    mojom::KeyringId keyring_id,
    const std::vector<HardwareAccountInfo>& accounts) {
  CHECK(IsBitcoinHardwareKeyring(keyring_id));
  ScopedDictPrefUpdate update(profile_prefs, kBraveWalletKeyrings);
  auto& hw_accounts =
      GetListPrefForKeyringUpdate(update, kAccountMetas, keyring_id);
  hw_accounts.clear();
  for (auto& account : accounts) {
    hw_accounts.Append(account.ToValue());
  }
}

void AddHardwareAccountToPrefs(PrefService* profile_prefs,
                               const HardwareAccountInfo& info) {
  CHECK(profile_prefs);
  CHECK(IsBitcoinHardwareKeyring(info.keyring_id));

  const auto keyring_id = info.keyring_id;
  auto accounts = GetHardwareAccountsForKeyring(profile_prefs, keyring_id);
  accounts.push_back(info);
  SetHardwareAccountsForKeyring(profile_prefs, keyring_id, accounts);
}

void RemoveHardwareAccountFromPrefs(PrefService* profile_prefs,
                                    const mojom::AccountId& account_id) {
  CHECK(profile_prefs);
  CHECK(IsBitcoinHardwareKeyring(account_id.keyring_id));

  auto accounts =
      GetHardwareAccountsForKeyring(profile_prefs, account_id.keyring_id);
  std::erase_if(accounts, [&](HardwareAccountInfo& acc) {
    return account_id == *acc.GetAccountId();
  });
  SetHardwareAccountsForKeyring(profile_prefs, account_id.keyring_id, accounts);
}

}  // namespace brave_wallet
