/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/keyring_service.h"

#include <array>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <utility>

#include "base/base64.h"
#include "base/check.h"
#include "base/check_is_test.h"
#include "base/check_op.h"
#include "base/command_line.h"
#include "base/containers/span.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/notreached.h"
#include "base/numerics/safe_conversions.h"
#include "base/rand_util.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/value_iterators.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/bip39.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_hardware_keyring.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_hd_keyring.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_import_keyring.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_cip30_serializer.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_hd_keyring.h"
#include "brave/components/brave_wallet/browser/ethereum_keyring.h"
#include "brave/components/brave_wallet/browser/filecoin_keyring.h"
#include "brave/components/brave_wallet/browser/json_keystore_parser.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/keyring_service_migrations.h"
#include "brave/components/brave_wallet/browser/keyring_service_prefs.h"
#include "brave/components/brave_wallet/browser/password_encryptor.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/browser/solana_keyring.h"
#include "brave/components/brave_wallet/browser/wallet_data_files_installer.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_keyring.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_constants.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/cardano_address.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "brave/components/brave_wallet/common/encoding_utils.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/fil_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "brave/components/brave_wallet/common/solana_utils.h"
#include "brave/components/brave_wallet/common/switches.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "ui/base/l10n/l10n_util.h"

namespace brave_wallet {
using mojom::KeyringId;

namespace {
constexpr char kHardwareAccounts[] = "hardware";
constexpr char kHardwareDerivationPath[] = "derivation_path";
constexpr char kHardwareVendor[] = "hardware_vendor";

std::string GetAccountName(size_t number) {
  return l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_NUMBERED_ACCOUNT_NAME,
                                   base::NumberToString16(number));
}

mojom::AccountInfoPtr MakeAccountInfoForHardwareAccount(
    const std::string& address,
    const std::string& name,
    const std::string& derivation_path,
    mojom::HardwareVendor hardware_vendor,
    const std::string& device_id,
    mojom::KeyringId keyring_id) {
  return mojom::AccountInfo::New(
      MakeAccountId(GetCoinForKeyring(keyring_id), keyring_id,
                    mojom::AccountKind::kHardware, address),
      address, name,
      mojom::HardwareInfo::New(derivation_path, hardware_vendor, device_id));
}

void SerializeHardwareAccounts(const std::string& device_id,
                               const base::Value* account_value,
                               mojom::KeyringId keyring_id,
                               std::vector<mojom::AccountInfoPtr>* accounts) {
  CHECK(!IsBitcoinKeyring(keyring_id));
  CHECK(!IsCardanoKeyring(keyring_id));
  CHECK(!IsZCashKeyring(keyring_id));
  for (const auto account : account_value->GetDict()) {
    DCHECK(account.second.is_dict());
    std::string address = account.first;
    const base::Value::Dict& dict = account.second.GetDict();

    mojom::HardwareVendor hardware_vendor = mojom::HardwareVendor::kLedger;
    const std::string* hardware_value = dict.FindString(kHardwareVendor);
    if (hardware_value) {
      if (*hardware_value == kLedgerPrefValue) {
        hardware_vendor = mojom::HardwareVendor::kLedger;
      } else if (*hardware_value == kTrezorPrefValue) {
        hardware_vendor = mojom::HardwareVendor::kTrezor;
      } else {
        continue;
      }
    }

    std::string name;
    const std::string* name_value = dict.FindString(kAccountName);
    if (name_value) {
      name = *name_value;
    }

    std::string derivation_path;
    const std::string* derivation_path_value =
        dict.FindString(kHardwareDerivationPath);
    if (derivation_path_value) {
      derivation_path = *derivation_path_value;
    }

    accounts->push_back(MakeAccountInfoForHardwareAccount(
        address, name, derivation_path, hardware_vendor, device_id,
        keyring_id));
  }
}

std::optional<std::string> DecryptWalletMnemonicFromPrefs(
    const PrefService* profile_prefs,
    PasswordEncryptor& encryptor) {
  CHECK(profile_prefs);

  if (auto decrypted = encryptor.DecryptFromDict(
          profile_prefs->GetDict(kBraveWalletMnemonic))) {
    return std::string(decrypted->begin(), decrypted->end());
  }

  return std::nullopt;
}

std::vector<uint8_t> GetSaltFromPrefs(const PrefService* profile_prefs) {
  CHECK(profile_prefs);

  auto salt_encoded = profile_prefs->GetString(kBraveWalletEncryptorSalt);
  if (!salt_encoded.empty()) {
    if (auto decoded = base::Base64Decode(salt_encoded)) {
      return *decoded;
    }
  }

  return {};
}

bool IsLegacyEthSeedFormat(const PrefService* profile_prefs) {
  CHECK(profile_prefs);
  return profile_prefs->GetBoolean(kBraveWalletLegacyEthSeedFormat);
}

// Utility structure that helps storing imported accounts in prefs.
struct ImportedAccountInfo {
  ImportedAccountInfo() = default;
  ImportedAccountInfo(mojom::KeyringId keyring_id,
                      std::string account_name,
                      std::optional<std::string> account_address,
                      uint32_t account_index,
                      base::Value::Dict imported_private_key)
      : keyring_id(keyring_id),
        account_name(std::move(account_name)),
        account_address(std::move(account_address)),
        account_index(account_index),
        imported_private_key(std::move(imported_private_key)) {}

  ~ImportedAccountInfo() = default;
  ImportedAccountInfo(const ImportedAccountInfo& other) = delete;
  ImportedAccountInfo& operator=(const ImportedAccountInfo& other) = delete;
  ImportedAccountInfo(ImportedAccountInfo&& other) = default;
  ImportedAccountInfo& operator=(ImportedAccountInfo&& other) = default;

  mojom::AccountIdPtr GetAccountId() const {
    if (!account_address) {
      return MakeIndexBasedAccountId(GetCoinForKeyring(keyring_id), keyring_id,
                                     mojom::AccountKind::kImported,
                                     account_index);
    }
    return MakeAccountId(GetCoinForKeyring(keyring_id), keyring_id,
                         mojom::AccountKind::kImported, *account_address);
  }

  base::Value ToValue() const {
    base::Value::Dict imported_account;
    imported_account.Set(kAccountName, account_name);
    if (account_address) {
      imported_account.Set(kAccountAddress, *account_address);
    } else {
      imported_account.Set(kAccountIndex,
                           base::checked_cast<int>(account_index));
    }
    imported_account.Set(kEncryptedPrivateKey, imported_private_key.Clone());
    if (bitcoin_next_receive_address_index) {
      imported_account.SetByDottedPath(
          kBitcoinNextReceiveIndex,
          base::NumberToString(*bitcoin_next_receive_address_index));
    }
    if (bitcoin_next_change_address_index) {
      imported_account.SetByDottedPath(
          kBitcoinNextChangeIndex,
          base::NumberToString(*bitcoin_next_change_address_index));
    }
    return base::Value(std::move(imported_account));
  }

  static std::optional<ImportedAccountInfo> FromValue(
      mojom::KeyringId keyring_id,
      const base::Value& value) {
    if (!value.is_dict()) {
      return std::nullopt;
    }
    auto& value_dict = value.GetDict();

    const std::string* account_name = value_dict.FindString(kAccountName);
    const std::string* account_address = value_dict.FindString(kAccountAddress);
    const std::optional<int> account_index = value_dict.FindInt(kAccountIndex);
    const base::Value::Dict* imported_private_key =
        value_dict.FindDict(kEncryptedPrivateKey);
    if (!account_name || !imported_private_key) {
      return std::nullopt;
    }

    if (!account_index && !account_address) {
      return std::nullopt;
    }

    ImportedAccountInfo account_info(
        keyring_id, *account_name,
        account_address ? std::make_optional(*account_address) : std::nullopt,
        account_index.value_or(0), imported_private_key->Clone());

    if (auto* bitcoin_receive_string =
            value_dict.FindStringByDottedPath(kBitcoinNextReceiveIndex)) {
      uint32_t bitcoin_receive_value = 0;
      if (!base::StringToUint(*bitcoin_receive_string,
                              &bitcoin_receive_value)) {
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

    return account_info;
  }

  mojom::KeyringId keyring_id;
  std::string account_name;
  std::optional<std::string> account_address;
  uint32_t account_index = 0;
  base::Value::Dict imported_private_key;
  std::optional<uint32_t> bitcoin_next_receive_address_index;
  std::optional<uint32_t> bitcoin_next_change_address_index;
};

// Gets all imported account from prefs.
std::vector<ImportedAccountInfo> GetImportedAccountsForKeyring(
    PrefService* profile_prefs,
    mojom::KeyringId keyring_id) {
  std::vector<ImportedAccountInfo> result;
  const base::Value::List* imported_accounts =
      GetPrefForKeyringList(profile_prefs, kImportedAccounts, keyring_id);
  if (!imported_accounts) {
    return result;
  }
  for (const auto& item : *imported_accounts) {
    if (auto imported_account =
            ImportedAccountInfo::FromValue(keyring_id, item)) {
      result.emplace_back(std::move(*imported_account));
    }
  }
  return result;
}

void SetImportedAccountsForKeyring(
    PrefService* profile_prefs,
    mojom::KeyringId keyring_id,
    const std::vector<ImportedAccountInfo>& accounts) {
  ScopedDictPrefUpdate update(profile_prefs, kBraveWalletKeyrings);
  auto& imported_accounts =
      GetListPrefForKeyringUpdate(update, kImportedAccounts, keyring_id);
  imported_accounts.clear();
  for (auto& account : accounts) {
    imported_accounts.Append(account.ToValue());
  }
}

// Adds imported account to prefs.
void AddImportedAccountForKeyring(PrefService* profile_prefs,
                                  ImportedAccountInfo info,
                                  mojom::KeyringId keyring_id) {
  CHECK(profile_prefs);

  auto accounts = GetImportedAccountsForKeyring(profile_prefs, keyring_id);
  accounts.push_back(std::move(info));
  SetImportedAccountsForKeyring(profile_prefs, keyring_id, accounts);
}

// Removes imported account from prefs by address or account_index.
void RemoveImportedAccountForKeyring(PrefService* profile_prefs,
                                     const mojom::AccountId& account_id,
                                     mojom::KeyringId keyring_id) {
  CHECK(profile_prefs);

  auto accounts = GetImportedAccountsForKeyring(profile_prefs, keyring_id);
  std::erase_if(accounts, [&](ImportedAccountInfo& acc) {
    return account_id == *acc.GetAccountId();
  });
  SetImportedAccountsForKeyring(profile_prefs, keyring_id, accounts);
}

// Utility structure that helps storing HD accounts in prefs.
struct DerivedAccountInfo {
  DerivedAccountInfo(mojom::KeyringId keyring_id,
                     uint32_t account_index,
                     std::string account_name,
                     std::string account_address)
      : keyring_id(keyring_id),
        account_index(account_index),
        account_name(std::move(account_name)),
        account_address(std::move(account_address)) {}

  ~DerivedAccountInfo() = default;
  DerivedAccountInfo(const DerivedAccountInfo& other) = default;

  mojom::AccountIdPtr GetAccountId() const {
    if (IsZCashKeyring(keyring_id)) {
      return MakeIndexBasedAccountId(GetCoinForKeyring(keyring_id), keyring_id,
                                     mojom::AccountKind::kDerived,
                                     account_index);
    }
    if (IsBitcoinKeyring(keyring_id)) {
      return MakeIndexBasedAccountId(GetCoinForKeyring(keyring_id), keyring_id,
                                     mojom::AccountKind::kDerived,
                                     account_index);
    }
    if (IsCardanoKeyring(keyring_id)) {
      return MakeIndexBasedAccountId(GetCoinForKeyring(keyring_id), keyring_id,
                                     mojom::AccountKind::kDerived,
                                     account_index);
    }
    return MakeAccountId(GetCoinForKeyring(keyring_id), keyring_id,
                         mojom::AccountKind::kDerived, account_address);
  }

  base::Value ToValue() const {
    base::Value::Dict derived_account;
    derived_account.Set(kAccountIndex, base::NumberToString(account_index));
    derived_account.Set(kAccountName, account_name);
    derived_account.Set(kAccountAddress, account_address);
    if (bitcoin_next_receive_address_index) {
      derived_account.SetByDottedPath(
          kBitcoinNextReceiveIndex,
          base::NumberToString(*bitcoin_next_receive_address_index));
    }
    if (bitcoin_next_change_address_index) {
      derived_account.SetByDottedPath(
          kBitcoinNextChangeIndex,
          base::NumberToString(*bitcoin_next_change_address_index));
    }
    if (zcash_account_birthday) {
      derived_account.SetByDottedPath(
          kZcashAccountBirthdayBlockId,
          base::NumberToString(zcash_account_birthday->first));
      derived_account.SetByDottedPath(kZcashAccountBirthdayBlockHash,
                                      zcash_account_birthday->second);
    }
    if (cardano_next_external_address_index) {
      derived_account.SetByDottedPath(
          kCardanoNextExternalIndex,
          base::NumberToString(*cardano_next_external_address_index));
    }
    if (cardano_next_internal_address_index) {
      derived_account.SetByDottedPath(
          kCardanoNextInternalIndex,
          base::NumberToString(*cardano_next_internal_address_index));
    }
    return base::Value(std::move(derived_account));
  }

  static std::optional<DerivedAccountInfo> FromValue(
      mojom::KeyringId keyring_id,
      const base::Value& value) {
    auto* value_dict = value.GetIfDict();
    if (!value_dict) {
      return std::nullopt;
    }
    const std::string* account_index_string =
        value_dict->FindString(kAccountIndex);
    const std::string* account_name = value_dict->FindString(kAccountName);
    const std::string* account_address =
        value_dict->FindString(kAccountAddress);
    if (!account_index_string || !account_name || !account_address) {
      return std::nullopt;
    }

    uint32_t account_index = 0;
    if (!base::StringToUint(*account_index_string, &account_index)) {
      return std::nullopt;
    }

    DerivedAccountInfo account_info(keyring_id, account_index, *account_name,
                                    *account_address);
    if (auto* bitcoin_receive_string =
            value_dict->FindStringByDottedPath(kBitcoinNextReceiveIndex)) {
      uint32_t bitcoin_receive_value = 0;
      if (!base::StringToUint(*bitcoin_receive_string,
                              &bitcoin_receive_value)) {
        return std::nullopt;
      }
      account_info.bitcoin_next_receive_address_index = bitcoin_receive_value;
    }
    if (auto* bitcoin_change_string =
            value_dict->FindStringByDottedPath(kBitcoinNextChangeIndex)) {
      uint32_t bitcoin_change_value = 0;
      if (!base::StringToUint(*bitcoin_change_string, &bitcoin_change_value)) {
        return std::nullopt;
      }
      account_info.bitcoin_next_change_address_index = bitcoin_change_value;
    }

    auto* zcash_account_birthday_block_id =
        value_dict->FindStringByDottedPath(kZcashAccountBirthdayBlockId);
    auto* zcash_account_birthday_hash =
        value_dict->FindStringByDottedPath(kZcashAccountBirthdayBlockHash);
    if (zcash_account_birthday_block_id && zcash_account_birthday_hash) {
      uint64_t zcash_account_birthday_value = 0;
      if (!base::StringToUint64(*zcash_account_birthday_block_id,
                                &zcash_account_birthday_value)) {
        return std::nullopt;
      }
      account_info.zcash_account_birthday = {zcash_account_birthday_value,
                                             *zcash_account_birthday_hash};
    }

    if (auto* cardano_external_string =
            value_dict->FindStringByDottedPath(kCardanoNextExternalIndex)) {
      uint32_t cardano_external_value = 0;
      if (!base::StringToUint(*cardano_external_string,
                              &cardano_external_value)) {
        return std::nullopt;
      }
      account_info.cardano_next_external_address_index = cardano_external_value;
    }
    if (auto* cardano_internal_string =
            value_dict->FindStringByDottedPath(kCardanoNextInternalIndex)) {
      uint32_t cardano_internal_value = 0;
      if (!base::StringToUint(*cardano_internal_string,
                              &cardano_internal_value)) {
        return std::nullopt;
      }
      account_info.cardano_next_internal_address_index = cardano_internal_value;
    }

    return account_info;
  }

  mojom::KeyringId keyring_id;
  uint32_t account_index;
  std::string account_name;
  std::string account_address;
  std::optional<uint32_t> bitcoin_next_receive_address_index;
  std::optional<uint32_t> bitcoin_next_change_address_index;
  std::optional<uint32_t> cardano_next_external_address_index;
  std::optional<uint32_t> cardano_next_internal_address_index;
  std::optional<std::pair<uint64_t, std::string>> zcash_account_birthday;
};

// Gets all hd account from prefs.
std::vector<DerivedAccountInfo> GetDerivedAccountsForKeyring(
    PrefService* profile_prefs,
    mojom::KeyringId keyring_id) {
  const base::Value::List* derived_accounts =
      GetPrefForKeyringList(profile_prefs, kAccountMetas, keyring_id);
  if (!derived_accounts) {
    return {};
  }

  std::vector<DerivedAccountInfo> result;
  for (auto& item : *derived_accounts) {
    if (auto derived_account =
            DerivedAccountInfo::FromValue(keyring_id, item)) {
      DCHECK_EQ(derived_account->account_index, result.size())
          << "No gaps allowed";
      result.emplace_back(std::move(*derived_account));
    }
  }

  return result;
}

void SetDerivedAccountsForKeyring(
    PrefService* profile_prefs,
    mojom::KeyringId keyring_id,
    const std::vector<DerivedAccountInfo>& accounts) {
  ScopedDictPrefUpdate update(profile_prefs, kBraveWalletKeyrings);
  auto& derived_accounts =
      GetListPrefForKeyringUpdate(update, kAccountMetas, keyring_id);
  derived_accounts.clear();
  for (auto& account : accounts) {
    DCHECK_EQ(account.account_index, derived_accounts.size())
        << "No gaps allowed";
    derived_accounts.Append(account.ToValue());
  }
}

size_t GetDerivedAccountsNumberForKeyring(PrefService* profile_prefs,
                                          mojom::KeyringId keyring_id) {
  return GetDerivedAccountsForKeyring(profile_prefs, keyring_id).size();
}

mojom::AccountInfoPtr MakeAccountInfoForDerivedAccount(
    const DerivedAccountInfo& derived_account_info) {
  // Ignore address from prefs for bitcoin.
  auto address = IsBitcoinKeyring(derived_account_info.keyring_id) ||
                         IsZCashKeyring(derived_account_info.keyring_id)
                     ? ""
                     : derived_account_info.account_address;

  return mojom::AccountInfo::New(derived_account_info.GetAccountId(),
                                 std::move(address),
                                 derived_account_info.account_name, nullptr);
}

mojom::AccountInfoPtr MakeAccountInfoForImportedAccount(
    const ImportedAccountInfo& imported_account_info) {
  return mojom::AccountInfo::New(
      imported_account_info.GetAccountId(),
      imported_account_info.account_address.value_or(""),
      imported_account_info.account_name, nullptr);
}

void AddDerivedAccountInfoForKeyring(PrefService* profile_prefs,
                                     const DerivedAccountInfo& account,
                                     mojom::KeyringId keyring_id) {
  auto accounts = GetDerivedAccountsForKeyring(profile_prefs, keyring_id);
  DCHECK_EQ(account.account_index, accounts.size()) << "No gaps allowed";
  accounts.push_back(account);
  SetDerivedAccountsForKeyring(profile_prefs, keyring_id, accounts);
}

std::string GetSelectedWalletAccountFromPrefs(PrefService* profile_prefs) {
  return profile_prefs->GetString(kBraveWalletSelectedWalletAccount);
}

template <class T>
bool UpdateBitcoinAccountIndexes(
    T& account,
    const std::optional<uint32_t>& next_receive_index,
    const std::optional<uint32_t>& next_change_index) {
  bool account_changed = false;
  if (next_receive_index) {
    account_changed =
        account_changed ||
        account.bitcoin_next_receive_address_index != *next_receive_index;
    account.bitcoin_next_receive_address_index = *next_receive_index;
  }
  if (next_change_index) {
    account_changed =
        account_changed ||
        account.bitcoin_next_change_address_index != *next_change_index;
    account.bitcoin_next_change_address_index = *next_change_index;
  }

  return account_changed;
}

template <class T>
mojom::BitcoinAccountInfoPtr BitcoinAccountInfoFromPrefInfo(
    BitcoinBaseKeyring& btc_base_keyring,
    const T& pref_info) {
  auto result = mojom::BitcoinAccountInfo::New();
  const auto next_receive_index =
      pref_info.bitcoin_next_receive_address_index.value_or(0);
  const auto next_change_index =
      pref_info.bitcoin_next_change_address_index.value_or(0);

  result->next_receive_address = btc_base_keyring.GetAddress(
      pref_info.account_index,
      mojom::BitcoinKeyId(kBitcoinReceiveIndex, next_receive_index));

  result->next_change_address = btc_base_keyring.GetAddress(
      pref_info.account_index,
      mojom::BitcoinKeyId(kBitcoinChangeIndex, next_change_index));

  if (!result->next_receive_address || !result->next_change_address) {
    return nullptr;
  }

  return result;
}

bool UpdateCardanoAccountIndexes(
    DerivedAccountInfo& account,
    const std::optional<uint32_t>& next_external_index,
    const std::optional<uint32_t>& next_internal_index) {
  bool account_changed = false;
  if (next_external_index) {
    account_changed =
        account_changed ||
        account.cardano_next_external_address_index != *next_external_index;
    DCHECK_GE(*next_external_index,
              account.cardano_next_external_address_index.value_or(0u));
    account.cardano_next_external_address_index = *next_external_index;
  }
  if (next_internal_index) {
    account_changed =
        account_changed ||
        account.cardano_next_internal_address_index != *next_internal_index;
    DCHECK_GE(*next_internal_index,
              account.cardano_next_internal_address_index.value_or(0u));
    account.cardano_next_internal_address_index = *next_internal_index;
  }

  return account_changed;
}

template <class T>
mojom::CardanoAccountInfoPtr CardanoAccountInfoFromPrefInfo(
    CardanoHDKeyring& cardano_base_keyring,
    const T& pref_info) {
  auto result = mojom::CardanoAccountInfo::New();
  const auto next_external_index =
      pref_info.cardano_next_external_address_index.value_or(0);
  const auto next_internal_index =
      pref_info.cardano_next_internal_address_index.value_or(0);

  result->next_external_address = cardano_base_keyring.GetAddress(
      pref_info.account_index,
      mojom::CardanoKeyId(mojom::CardanoKeyRole::kExternal,
                          next_external_index));

  result->next_internal_address = cardano_base_keyring.GetAddress(
      pref_info.account_index,
      mojom::CardanoKeyId(mojom::CardanoKeyRole::kInternal,
                          next_internal_index));

  if (!result->next_external_address || !result->next_internal_address) {
    return nullptr;
  }

  return result;
}

}  // namespace

struct KeyringSeed {
  std::vector<uint8_t> eth_seed;
  std::vector<uint8_t> seed;
  std::vector<uint8_t> entropy;
};

std::optional<KeyringSeed> MakeSeedFromMnemonic(
    const std::string& mnemonic,
    bool use_legacy_eth_seed_format) {
  KeyringSeed result;
  const auto seed = bip39::MnemonicToSeed(mnemonic, "");
  if (!seed) {
    return std::nullopt;
  }

  result.seed = std::move(*seed);

  auto entropy = bip39::MnemonicToEntropy(mnemonic);
  if (!entropy) {
    return std::nullopt;
  }

  if (use_legacy_eth_seed_format) {
    if (entropy->size() != bip39::kLegacyEthEntropySize) {
      return std::nullopt;
    }
    result.eth_seed = *entropy;
  } else {
    result.eth_seed = result.seed;
  }

  result.entropy = *entropy;

  return result;
}

KeyringService::KeyringService(JsonRpcService* json_rpc_service,
                               PrefService* profile_prefs,
                               PrefService* local_state)
    : json_rpc_service_(json_rpc_service),
      profile_prefs_(profile_prefs),
      local_state_(local_state) {
  DCHECK(profile_prefs);
  DCHECK(local_state);
  auto_lock_timer_ = std::make_unique<base::OneShotTimer>();

  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(profile_prefs);
  pref_change_registrar_->Add(
      kBraveWalletAutoLockMinutes,
      base::BindRepeating(&KeyringService::OnAutoLockPreferenceChanged,
                          base::Unretained(this)));

  enabled_keyrings_ = GetEnabledKeyrings();

  MaybeUnlockWithCommandLine();
}

KeyringService::~KeyringService() {
  auto_lock_timer_.reset();
}

void KeyringService::Bind(
    mojo::PendingReceiver<mojom::KeyringService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void KeyringService::RequestUnlock() {
  DCHECK(IsLockedSync());
  request_unlock_pending_ = true;
}

void KeyringService::GetWalletMnemonic(const std::string& password,
                                       GetWalletMnemonicCallback callback) {
  if (IsLockedSync()) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  std::move(callback).Run(GetWalletMnemonicInternal(password));
}

void KeyringService::IsWalletCreated(IsWalletCreatedCallback callback) {
  std::move(callback).Run(IsWalletCreatedSync());
}

bool KeyringService::IsWalletCreatedSync() {
  return profile_prefs_->GetDict(kBraveWalletKeyrings).size() > 0;
}

void KeyringService::CreateWallet(const std::string& password,
                                  CreateWalletCallback callback) {
  const auto mnemonic = bip39::GenerateMnemonic(
      base::RandBytesAsVector(bip39::kDefaultEntropySize));
  if (!mnemonic) {
    std::move(callback).Run(std::nullopt);
    return;
  }
  if (CreateWalletInternal(*mnemonic, password, false, false)) {
    WalletDataFilesInstaller::GetInstance()
        .MaybeRegisterWalletDataFilesComponentOnDemand(base::BindOnce(
            [](const std::string& mnemonic, CreateWalletCallback callback) {
              std::move(callback).Run(mnemonic);
            },
            *mnemonic, std::move(callback)));
  } else {
    std::move(callback).Run(std::nullopt);
  }
}

bool KeyringService::CreateWalletInternal(const std::string& mnemonic,
                                          const std::string& password,
                                          bool is_legacy_eth_seed_format,
                                          bool from_restore) {
  if (encryptor_) {
    return false;
  }
  auto keyring_seed = MakeSeedFromMnemonic(mnemonic, is_legacy_eth_seed_format);
  if (!keyring_seed) {
    return false;
  }

  auto salt = PasswordEncryptor::CreateSalt();
  auto encryptor = PasswordEncryptor::CreateEncryptor(password, salt);
  if (!encryptor) {
    return false;
  }
  encryptor_ = std::move(encryptor);

  profile_prefs_->SetBoolean(kBraveWalletKeyringEncryptionKeysMigrated, true);
  profile_prefs_->SetBoolean(kBraveWalletLegacyEthSeedFormat,
                             is_legacy_eth_seed_format);
  profile_prefs_->SetDict(
      kBraveWalletMnemonic,
      encryptor_->EncryptToDict(base::as_byte_span(mnemonic),
                                PasswordEncryptor::CreateNonce()));
  profile_prefs_->SetString(kBraveWalletEncryptorSalt,
                            base::Base64Encode(salt));

  CreateKeyrings(*keyring_seed);
  CreateDefaultAccounts();

  for (const auto& observer : observers_) {
    if (from_restore) {
      observer->WalletRestored();
    } else {
      observer->WalletCreated();
    }
  }

  ResetAutoLockTimer();
  UpdateLastUnlockPref(local_state_);

  return true;
}

bool KeyringService::IsKeyringEnabled(mojom::KeyringId keyring_id) const {
  return base::Contains(enabled_keyrings_, keyring_id);
}

void KeyringService::CreateKeyrings(const KeyringSeed& keyring_seed) {
  ethereum_keyring_ = std::make_unique<EthereumKeyring>(keyring_seed.eth_seed);

  solana_keyring_ = std::make_unique<SolanaKeyring>(keyring_seed.seed);

  filecoin_mainnet_keyring_ = std::make_unique<FilecoinKeyring>(
      keyring_seed.seed, KeyringId::kFilecoin);
  filecoin_testnet_keyring_ = std::make_unique<FilecoinKeyring>(
      keyring_seed.seed, KeyringId::kFilecoinTestnet);

  if (IsKeyringEnabled(KeyringId::kBitcoin84)) {
    bitcoin_hd_mainnet_keyring_ = std::make_unique<BitcoinHDKeyring>(
        keyring_seed.seed, KeyringId::kBitcoin84);
  }
  if (IsKeyringEnabled(KeyringId::kBitcoin84Testnet)) {
    bitcoin_hd_testnet_keyring_ = std::make_unique<BitcoinHDKeyring>(
        keyring_seed.seed, KeyringId::kBitcoin84Testnet);
  }
  if (IsKeyringEnabled(KeyringId::kBitcoinImport)) {
    bitcoin_import_mainnet_keyring_ =
        std::make_unique<BitcoinImportKeyring>(KeyringId::kBitcoinImport);
  }
  if (IsKeyringEnabled(KeyringId::kBitcoinImportTestnet)) {
    bitcoin_import_testnet_keyring_ = std::make_unique<BitcoinImportKeyring>(
        KeyringId::kBitcoinImportTestnet);
  }
  if (IsKeyringEnabled(KeyringId::kBitcoinHardware)) {
    bitcoin_hardware_mainnet_keyring_ =
        std::make_unique<BitcoinHardwareKeyring>(KeyringId::kBitcoinHardware);
  }
  if (IsKeyringEnabled(KeyringId::kBitcoinHardwareTestnet)) {
    bitcoin_hardware_testnet_keyring_ =
        std::make_unique<BitcoinHardwareKeyring>(
            KeyringId::kBitcoinHardwareTestnet);
  }

  if (IsKeyringEnabled(KeyringId::kZCashMainnet)) {
    zcash_hd_mainnet_keyring_ = std::make_unique<ZCashKeyring>(
        keyring_seed.seed, KeyringId::kZCashMainnet);
  }
  if (IsKeyringEnabled(KeyringId::kZCashTestnet)) {
    zcash_hd_testnet_keyring_ = std::make_unique<ZCashKeyring>(
        keyring_seed.seed, KeyringId::kZCashTestnet);
  }

  if (IsKeyringEnabled(KeyringId::kCardanoMainnet)) {
    cardano_hd_mainnet_keyring_ = std::make_unique<CardanoHDKeyring>(
        keyring_seed.entropy, KeyringId::kCardanoMainnet);
  }
  if (IsKeyringEnabled(KeyringId::kCardanoTestnet)) {
    cardano_hd_testnet_keyring_ = std::make_unique<CardanoHDKeyring>(
        keyring_seed.entropy, KeyringId::kCardanoTestnet);
  }
}

void KeyringService::ClearKeyrings() {
  ethereum_keyring_.reset();

  solana_keyring_.reset();

  filecoin_mainnet_keyring_.reset();
  filecoin_testnet_keyring_.reset();

  bitcoin_hd_mainnet_keyring_.reset();
  bitcoin_hd_testnet_keyring_.reset();
  bitcoin_import_mainnet_keyring_.reset();
  bitcoin_import_testnet_keyring_.reset();
  bitcoin_hardware_mainnet_keyring_.reset();
  bitcoin_hardware_testnet_keyring_.reset();

  zcash_hd_mainnet_keyring_.reset();
  zcash_hd_testnet_keyring_.reset();

  cardano_hd_mainnet_keyring_.reset();
  cardano_hd_testnet_keyring_.reset();
}

void KeyringService::CreateDefaultAccounts() {
  if (auto account =
          AddHDAccountForKeyring(mojom::kDefaultKeyringId, GetAccountName(1))) {
    SetSelectedAccountInternal(*account);
    NotifyAccountsAdded(*account);
  }
  if (auto account = AddHDAccountForKeyring(mojom::kSolanaKeyringId,
                                            "Solana " + GetAccountName(1))) {
    SetSelectedAccountInternal(*account);
    NotifyAccountsAdded(*account);
  }
  ResetAllAccountInfosCache();
}

void KeyringService::LoadAllAccountsFromPrefs() {
  for (auto keyring_id : GetEnabledKeyrings()) {
    LoadAccountsFromPrefs(keyring_id);
  }
}

template <>
EthereumKeyring* KeyringService::GetKeyring(mojom::KeyringId keyring_id) const {
  if (IsEthereumKeyring(keyring_id)) {
    return ethereum_keyring_.get();
  }
  return nullptr;
}

template <>
SolanaKeyring* KeyringService::GetKeyring(mojom::KeyringId keyring_id) const {
  if (IsSolanaKeyring(keyring_id)) {
    return solana_keyring_.get();
  }
  return nullptr;
}

template <>
FilecoinKeyring* KeyringService::GetKeyring(mojom::KeyringId keyring_id) const {
  for (auto* keyring :
       {filecoin_mainnet_keyring_.get(), filecoin_testnet_keyring_.get()}) {
    if (keyring && keyring->keyring_id() == keyring_id) {
      return keyring;
    }
  }
  return nullptr;
}

template <>
EthereumKeyring* KeyringService::GetKeyring(
    const mojom::AccountIdPtr& account_id) const {
  if (IsEthereumAccount(account_id)) {
    return GetKeyring<EthereumKeyring>(account_id->keyring_id);
  }
  return nullptr;
}

template <>
SolanaKeyring* KeyringService::GetKeyring(
    const mojom::AccountIdPtr& account_id) const {
  if (IsSolanaAccount(account_id)) {
    return GetKeyring<SolanaKeyring>(account_id->keyring_id);
  }
  return nullptr;
}

template <>
FilecoinKeyring* KeyringService::GetKeyring(
    const mojom::AccountIdPtr& account_id) const {
  if (IsFilecoinAccount(account_id)) {
    return GetKeyring<FilecoinKeyring>(account_id->keyring_id);
  }
  return nullptr;
}

template <>
BitcoinImportKeyring* KeyringService::GetKeyring(
    mojom::KeyringId keyring_id) const {
  for (auto* keyring : {bitcoin_import_mainnet_keyring_.get(),
                        bitcoin_import_testnet_keyring_.get()}) {
    if (keyring && keyring->keyring_id() == keyring_id) {
      return keyring;
    }
  }
  return nullptr;
}

template <>
BitcoinHardwareKeyring* KeyringService::GetKeyring(
    mojom::KeyringId keyring_id) const {
  for (auto* keyring : {bitcoin_hardware_mainnet_keyring_.get(),
                        bitcoin_hardware_testnet_keyring_.get()}) {
    if (keyring && keyring->keyring_id() == keyring_id) {
      return keyring;
    }
  }
  return nullptr;
}

template <>
BitcoinHDKeyring* KeyringService::GetKeyring(
    mojom::KeyringId keyring_id) const {
  for (auto* keyring :
       {bitcoin_hd_mainnet_keyring_.get(), bitcoin_hd_testnet_keyring_.get()}) {
    if (keyring && keyring->keyring_id() == keyring_id) {
      return keyring;
    }
  }
  return nullptr;
}

template <>
BitcoinBaseKeyring* KeyringService::GetKeyring(
    mojom::KeyringId keyring_id) const {
  if (IsBitcoinHDKeyring(keyring_id)) {
    return GetKeyring<BitcoinHDKeyring>(keyring_id);
  }

  if (IsBitcoinImportKeyring(keyring_id)) {
    return GetKeyring<BitcoinImportKeyring>(keyring_id);
  }

  if (IsBitcoinHardwareKeyring(keyring_id)) {
    return GetKeyring<BitcoinHardwareKeyring>(keyring_id);
  }

  return nullptr;
}

template <>
CardanoHDKeyring* KeyringService::GetKeyring(
    mojom::KeyringId keyring_id) const {
  for (auto* keyring :
       {cardano_hd_mainnet_keyring_.get(), cardano_hd_testnet_keyring_.get()}) {
    if (keyring && keyring->keyring_id() == keyring_id) {
      return keyring;
    }
  }

  return nullptr;
}

template <>
ZCashKeyring* KeyringService::GetKeyring(mojom::KeyringId keyring_id) const {
  for (auto* keyring :
       {zcash_hd_mainnet_keyring_.get(), zcash_hd_testnet_keyring_.get()}) {
    if (keyring && keyring->keyring_id() == keyring_id) {
      return keyring;
    }
  }

  return nullptr;
}

void KeyringService::LoadAccountsFromPrefs(mojom::KeyringId keyring_id) {
  CHECK(encryptor_);

  if (IsBitcoinImportKeyring(keyring_id)) {
    auto* keyring = GetKeyring<BitcoinImportKeyring>(keyring_id);
    CHECK(keyring);
    for (const auto& imported_account_info :
         GetImportedAccountsForKeyring(profile_prefs_, keyring_id)) {
      auto private_key = encryptor_->DecryptFromDict(
          imported_account_info.imported_private_key);
      if (!private_key) {
        continue;
      }
      CHECK(!imported_account_info.account_address);
      keyring->AddAccount(
          imported_account_info.account_index,
          std::string(private_key->begin(), private_key->end()));
    }
    return;
  }

  if (IsBitcoinHardwareKeyring(keyring_id)) {
    auto* keyring = GetKeyring<BitcoinHardwareKeyring>(keyring_id);
    CHECK(keyring);
    for (const auto& hardware_account_info :
         GetHardwareAccountsForKeyring(profile_prefs_, keyring_id)) {
      CHECK(hardware_account_info.bitcoin_xpub);
      keyring->AddAccount(hardware_account_info.account_index,
                          *hardware_account_info.bitcoin_xpub);
    }
    return;
  }

  size_t account_no =
      GetDerivedAccountsNumberForKeyring(profile_prefs_, keyring_id);
  for (auto i = 0u; i < account_no; ++i) {
    AddHDAccountForKeyringInternal(keyring_id, i);
  }

  for (const auto& imported_account_info :
       GetImportedAccountsForKeyring(profile_prefs_, keyring_id)) {
    if (!imported_account_info.account_address) {
      continue;
    }

    auto private_key =
        encryptor_->DecryptFromDict(imported_account_info.imported_private_key);
    if (!private_key) {
      continue;
    }

    if (auto* ethereum_keyring = GetKeyring<EthereumKeyring>(keyring_id)) {
      ethereum_keyring->ImportAccount(*private_key);
    } else if (auto* solana_keyring = GetKeyring<SolanaKeyring>(keyring_id)) {
      solana_keyring->ImportAccount(*private_key);
    } else if (auto* filecoin_keyring =
                   GetKeyring<FilecoinKeyring>(keyring_id)) {
      if (auto protocol = FilAddress::GetProtocolFromAddress(
              *imported_account_info.account_address)) {
        auto imported_address =
            filecoin_keyring->ImportFilecoinAccount(*private_key, *protocol);
        DCHECK_EQ(*imported_account_info.account_address, *imported_address);
      }
    }
  }
}

void KeyringService::RestoreWallet(const std::string& mnemonic,
                                   const std::string& password,
                                   bool is_legacy_eth_seed_format,
                                   RestoreWalletCallback callback) {
  bool is_valid_mnemonic =
      RestoreWalletSync(mnemonic, password, is_legacy_eth_seed_format);
  if (!is_valid_mnemonic) {
    std::move(callback).Run(false);
    return;
  }

  // Only register the component if restore is successful.
  WalletDataFilesInstaller::GetInstance()
      .MaybeRegisterWalletDataFilesComponentOnDemand(base::BindOnce(
          [](RestoreWalletCallback callback) { std::move(callback).Run(true); },
          std::move(callback)));
}

bool KeyringService::CanResumeWallet(const std::string& mnemonic,
                                     const std::string& password,
                                     bool is_legacy_eth_seed_format) {
  auto current_mnemonic = GetWalletMnemonicInternal(password);
  if (!current_mnemonic) {
    return false;
  }

  return (*current_mnemonic == mnemonic &&
          IsLegacyEthSeedFormat(profile_prefs_) == is_legacy_eth_seed_format);
}

bool KeyringService::RestoreWalletSync(const std::string& mnemonic,
                                       const std::string& password,
                                       bool is_legacy_eth_seed_format) {
  MaybeRunPasswordMigrations(profile_prefs_, password);

  if (CanResumeWallet(mnemonic, password, is_legacy_eth_seed_format)) {
    Unlock(password, base::DoNothing());
    return true;
  }

  Reset(false);
  return CreateWalletInternal(mnemonic, password, is_legacy_eth_seed_format,
                              true);
}

void KeyringService::AddAccount(mojom::CoinType coin,
                                mojom::KeyringId keyring_id,
                                const std::string& account_name,
                                AddAccountCallback callback) {
  std::move(callback).Run(AddAccountSync(coin, keyring_id, account_name));
}

mojom::AccountInfoPtr KeyringService::AddAccountSync(
    mojom::CoinType coin,
    mojom::KeyringId keyring_id,
    const std::string& account_name) {
  auto account = AddHDAccountForKeyring(keyring_id, account_name);
  if (!account) {
    return nullptr;
  }
  NotifyAccountsChanged();

  // TODO(apaymyshev): ui should select account after creating account.
  SetSelectedAccountInternal(*account);
  NotifyAccountsAdded(*account);

  return account;
}

void KeyringService::EncodePrivateKeyForExport(
    mojom::AccountIdPtr account_id,
    const std::string& password,
    EncodePrivateKeyForExportCallback callback) {
  if (!account_id || !ValidatePasswordInternal(password)) {
    std::move(callback).Run(std::nullopt);
    return;
  }

  if (auto* keyring = GetKeyring<EthereumKeyring>(account_id)) {
    std::move(callback).Run(
        keyring->EncodePrivateKeyForExport(account_id->address));
    return;
  }

  if (auto* keyring = GetKeyring<SolanaKeyring>(account_id)) {
    std::move(callback).Run(
        keyring->EncodePrivateKeyForExport(account_id->address));
    return;
  }

  if (auto* keyring = GetKeyring<FilecoinKeyring>(account_id)) {
    std::move(callback).Run(
        keyring->EncodePrivateKeyForExport(account_id->address));
    return;
  }

  std::move(callback).Run(std::nullopt);
}

void KeyringService::ImportFilecoinAccount(
    const std::string& account_name,
    const std::string& private_key_hex,
    const std::string& network,
    ImportFilecoinAccountCallback callback) {
  if (account_name.empty() || private_key_hex.empty() || IsLockedSync()) {
    std::move(callback).Run({});
    return;
  }
  CHECK(encryptor_);

  std::vector<uint8_t> private_key;
  mojom::FilecoinAddressProtocol protocol;
  if (!FilecoinKeyring::DecodeImportPayload(private_key_hex, &private_key,
                                            &protocol)) {
    std::move(callback).Run({});
    return;
  }

  const mojom::KeyringId keyring_id = GetFilecoinKeyringId(network);
  auto* keyring = GetKeyring<FilecoinKeyring>(keyring_id);
  if (!keyring) {
    std::move(callback).Run({});
    return;
  }

  auto address = keyring->ImportFilecoinAccount(private_key, protocol);
  if (!address) {
    std::move(callback).Run({});
    return;
  }

  ImportedAccountInfo imported_account_info(
      keyring_id, account_name, *address, 0,
      encryptor_->EncryptToDict(private_key, PasswordEncryptor::CreateNonce()));

  auto account_info = MakeAccountInfoForImportedAccount(imported_account_info);

  AddImportedAccountForKeyring(profile_prefs_, std::move(imported_account_info),
                               keyring_id);
  NotifyAccountsChanged();

  // TODO(apaymyshev): ui should select account after importing.
  SetSelectedAccountInternal(*account_info);

  NotifyAccountsAdded(*account_info);

  std::move(callback).Run(std::move(account_info));
}

void KeyringService::ImportBitcoinAccount(
    const std::string& account_name,
    const std::string& payload,
    const std::string& network,
    ImportBitcoinAccountCallback callback) {
  std::move(callback).Run(
      ImportBitcoinAccountSync(account_name, payload, network));
}

mojom::AccountInfoPtr KeyringService::ImportBitcoinAccountSync(
    const std::string& account_name,
    const std::string& payload,
    const std::string& network) {
  if (account_name.empty() || payload.empty() || IsLockedSync() ||
      !IsBitcoinNetwork(network)) {
    return nullptr;
  }
  CHECK(encryptor_);

  const auto keyring_id = network == mojom::kBitcoinMainnet
                              ? mojom::KeyringId::kBitcoinImport
                              : mojom::KeyringId::kBitcoinImportTestnet;

  auto* keyring = GetKeyring<BitcoinImportKeyring>(keyring_id);

  if (!keyring) {
    return nullptr;
  }

  uint32_t account_index = GenerateNextAccountIndex(profile_prefs_, keyring_id);
  if (!keyring->AddAccount(account_index, payload)) {
    return nullptr;
  }

  ImportedAccountInfo imported_account_info(
      keyring_id, account_name, std::nullopt, account_index,
      encryptor_->EncryptToDict(base::as_byte_span(payload),
                                PasswordEncryptor::CreateNonce()));

  auto account_info = MakeAccountInfoForImportedAccount(imported_account_info);

  AddImportedAccountForKeyring(profile_prefs_, std::move(imported_account_info),
                               keyring_id);
  NotifyAccountsChanged();

  // TODO(apaymyshev): ui should select account after importing.
  SetSelectedAccountInternal(*account_info);

  NotifyAccountsAdded(*account_info);

  return account_info;
}

void KeyringService::ImportEthereumAccount(
    const std::string& account_name,
    const std::string& private_key,
    ImportEthereumAccountCallback callback) {
  std::string private_key_trimmed;
  base::TrimString(private_key, " \n\t", &private_key_trimmed);

  if (account_name.empty() || private_key.empty() || IsLockedSync()) {
    std::move(callback).Run({});
    return;
  }

  std::vector<uint8_t> private_key_bytes;
  if (!base::HexStringToBytes(private_key_trimmed, &private_key_bytes)) {
    // try again with 0x prefix considered
    if (!PrefixedHexStringToBytes(private_key_trimmed, &private_key_bytes)) {
      std::move(callback).Run({});
      return;
    }
  }

  if (private_key_bytes.empty()) {
    std::move(callback).Run({});
    return;
  }

  std::move(callback).Run(ImportAccountForKeyring(
      mojom::KeyringId::kDefault, account_name, private_key_bytes));
}

void KeyringService::ImportSolanaAccount(const std::string& account_name,
                                         const std::string& private_key,
                                         ImportSolanaAccountCallback callback) {
  std::string private_key_trimmed;
  base::TrimString(private_key, " \n\t", &private_key_trimmed);

  if (account_name.empty() || private_key.empty() || IsLockedSync()) {
    std::move(callback).Run({});
    return;
  }

  std::vector<uint8_t> keypair(kSolanaKeypairSize);
  if (!Base58Decode(private_key_trimmed, &keypair, keypair.size())) {
    if (!Uint8ArrayDecode(private_key_trimmed, &keypair, kSolanaKeypairSize)) {
      std::move(callback).Run({});
      return;
    }
  }

  if (keypair.empty()) {
    std::move(callback).Run({});
    return;
  }

  std::move(callback).Run(ImportAccountForKeyring(mojom::KeyringId::kSolana,
                                                  account_name, keypair));
}

void KeyringService::ImportEthereumAccountFromJson(
    const std::string& account_name,
    const std::string& password,
    const std::string& json,
    ImportEthereumAccountFromJsonCallback callback) {
  if (account_name.empty() || password.empty() || json.empty() ||
      IsLockedSync()) {
    std::move(callback).Run({});
    return;
  }

  auto parsed_json = base::JSONReader::ReadAndReturnValueWithError(json);
  if (!parsed_json.has_value() || !parsed_json->is_dict()) {
    std::move(callback).Run({});
    return;
  }

  CHECK(encryptor_);
  auto private_key =
      DecryptPrivateKeyFromJsonKeystore(password, parsed_json->GetDict());
  if (!private_key) {
    std::move(callback).Run({});
    return;
  }

  std::move(callback).Run(ImportAccountForKeyring(mojom::kDefaultKeyringId,
                                                  account_name, *private_key));
}

void KeyringService::SetSelectedAccountInternal(
    const mojom::AccountInfo& account_info) {
  SetSelectedWalletAccountInternal(account_info);

  if (CoinSupportsDapps(account_info.account_id->coin)) {
    SetSelectedDappAccountInternal(account_info.account_id->coin,
                                   account_info.Clone());
  }
}

void KeyringService::SetSelectedWalletAccountInternal(
    const mojom::AccountInfo& account_info) {
  const auto& account_id = *account_info.account_id;

  if (SetSelectedWalletAccountInPrefs(profile_prefs_, account_id.unique_key)) {
    NotifySelectedWalletAccountChanged(account_info);

    // TODO(apaymyshev): this should not be a part of KeyringService.
    if (account_id.coin == mojom::CoinType::FIL) {
      json_rpc_service_->SetNetwork(
          account_id.keyring_id == mojom::kFilecoinKeyringId
              ? mojom::kFilecoinMainnet
              : mojom::kFilecoinTestnet,
          account_id.coin, std::nullopt);
    }
  }
}

void KeyringService::SetSelectedDappAccountInternal(
    mojom::CoinType coin,
    const mojom::AccountInfoPtr& account_info) {
  CHECK(CoinSupportsDapps(coin));
  CHECK(!account_info || account_info->account_id->coin == coin);

  if (SetSelectedDappAccountInPrefs(
          profile_prefs_, coin,
          account_info ? account_info->account_id->unique_key : "")) {
    NotifySelectedDappAccountChanged(coin, account_info);
  }
}

void KeyringService::RemoveAccount(mojom::AccountIdPtr account_id,
                                   const std::string& password,
                                   RemoveAccountCallback callback) {
  if (account_id->kind == mojom::AccountKind::kImported &&
      !ValidatePasswordInternal(password)) {
    std::move(callback).Run(false);
    return;
  }

  if (account_id->kind == mojom::AccountKind::kImported) {
    std::move(callback).Run(RemoveImportedAccountInternal(account_id));
    return;
  }

  if (account_id->kind == mojom::AccountKind::kHardware) {
    std::move(callback).Run(RemoveHardwareAccountInternal(*account_id));
    return;
  }

  NOTREACHED() << account_id->kind;
}

bool KeyringService::RemoveImportedAccountInternal(
    const mojom::AccountIdPtr& account_id) {
  DCHECK_EQ(account_id->kind, mojom::AccountKind::kImported);

  if (auto* keyring =
          GetKeyring<BitcoinImportKeyring>(account_id->keyring_id)) {
    if (!keyring->RemoveAccount(account_id->account_index)) {
      return false;
    }
  }

  if (auto* ethereum_keyring = GetKeyring<EthereumKeyring>(account_id)) {
    if (!ethereum_keyring->RemoveImportedAccount(account_id->address)) {
      return false;
    }
  }

  if (auto* solana_keyring = GetKeyring<SolanaKeyring>(account_id)) {
    if (!solana_keyring->RemoveImportedAccount(account_id->address)) {
      return false;
    }
  }

  if (auto* filecoin_keyring = GetKeyring<FilecoinKeyring>(account_id)) {
    if (!filecoin_keyring->RemoveImportedAccount(account_id->address)) {
      return false;
    }
  }

  RemoveImportedAccountForKeyring(profile_prefs_, *account_id,
                                  account_id->keyring_id);

  NotifyAccountsChanged();
  MaybeFixAccountSelection();
  return true;
}

void KeyringService::IsWalletBackedUp(IsWalletBackedUpCallback callback) {
  std::move(callback).Run(IsWalletBackedUpSync());
}

bool KeyringService::IsWalletBackedUpSync() {
  return profile_prefs_->GetBoolean(kBraveWalletMnemonicBackedUp);
}

void KeyringService::NotifyWalletBackupComplete() {
  profile_prefs_->SetBoolean(kBraveWalletMnemonicBackedUp, true);
  for (const auto& observer : observers_) {
    observer->BackedUp();
  }
}

mojom::AccountInfoPtr KeyringService::AddHDAccountForKeyring(
    mojom::KeyringId keyring_id,
    const std::string& account_name) {
  size_t new_account_index =
      GetDerivedAccountsNumberForKeyring(profile_prefs_, keyring_id);
  auto address = AddHDAccountForKeyringInternal(keyring_id, new_account_index);
  if (!address) {
    return nullptr;
  }

  DerivedAccountInfo derived_account_info(keyring_id, new_account_index,
                                          account_name, *address);

  AddDerivedAccountInfoForKeyring(profile_prefs_, derived_account_info,
                                  keyring_id);

  return MakeAccountInfoForDerivedAccount(derived_account_info);
}

std::optional<std::string> KeyringService::AddHDAccountForKeyringInternal(
    mojom::KeyringId keyring_id,
    uint32_t index) {
  if (auto* keyring = GetKeyring<EthereumKeyring>(keyring_id)) {
    return keyring->AddNewHDAccount(index);
  }

  if (auto* keyring = GetKeyring<SolanaKeyring>(keyring_id)) {
    return keyring->AddNewHDAccount(index);
  }

  if (auto* keyring = GetKeyring<FilecoinKeyring>(keyring_id)) {
    return keyring->AddNewHDAccount(index);
  }

  if (auto* keyring = GetKeyring<BitcoinHDKeyring>(keyring_id)) {
    return keyring->AddNewHDAccount(index);
  }

  if (auto* keyring = GetKeyring<ZCashKeyring>(keyring_id)) {
    return keyring->AddNewHDAccount(index);
  }

  if (auto* keyring = GetKeyring<CardanoHDKeyring>(keyring_id)) {
    return keyring->AddNewHDAccount(index);
  }

  return std::nullopt;
}

mojom::AccountInfoPtr KeyringService::ImportAccountForKeyring(
    mojom::KeyringId keyring_id,
    const std::string& account_name,
    base::span<const uint8_t> private_key) {
  std::optional<std::string> address;
  if (auto* ethereum_keyring = GetKeyring<EthereumKeyring>(keyring_id)) {
    address = ethereum_keyring->ImportAccount(private_key);
  } else if (auto* solana_keyring = GetKeyring<SolanaKeyring>(keyring_id)) {
    address = solana_keyring->ImportAccount(private_key);
  } else {
    NOTREACHED() << keyring_id;
  }

  if (!address) {
    return nullptr;
  }

  CHECK(encryptor_);

  ImportedAccountInfo imported_account_info(
      keyring_id, account_name, *address, 0,
      encryptor_->EncryptToDict(private_key, PasswordEncryptor::CreateNonce()));
  auto account_info = MakeAccountInfoForImportedAccount(imported_account_info);
  AddImportedAccountForKeyring(profile_prefs_, std::move(imported_account_info),
                               keyring_id);
  NotifyAccountsChanged();

  // TODO(apaymyshev): ui should select account after importing.
  SetSelectedAccountInternal(*account_info);

  NotifyAccountsAdded(*account_info);
  return account_info;
}

// This member function should not assume that the wallet is unlocked!
std::vector<mojom::AccountInfoPtr> KeyringService::GetAccountInfosForKeyring(
    mojom::KeyringId keyring_id) const {
  std::vector<mojom::AccountInfoPtr> result;

  // Append HD accounts.
  for (const auto& derived_account_info :
       GetDerivedAccountsForKeyring(profile_prefs_, keyring_id)) {
    result.push_back(MakeAccountInfoForDerivedAccount(derived_account_info));
  }

  // Append imported accounts.
  for (const auto& imported_account_info :
       GetImportedAccountsForKeyring(profile_prefs_, keyring_id)) {
    result.push_back(MakeAccountInfoForImportedAccount(imported_account_info));
  }

  // Append hardware accounts.
  for (const auto& hardware_account_info :
       GetHardwareAccountsSync(keyring_id)) {
    result.push_back(hardware_account_info.Clone());
  }
  return result;
}

std::vector<mojom::AccountInfoPtr> KeyringService::GetHardwareAccountsSync(
    mojom::KeyringId keyring_id) const {
  std::vector<mojom::AccountInfoPtr> accounts;

  if (IsBitcoinKeyring(keyring_id)) {
    if (IsBitcoinLedgerEnabled() && IsBitcoinHardwareKeyring(keyring_id)) {
      for (auto& hardware_account :
           GetHardwareAccountsForKeyring(profile_prefs_, keyring_id)) {
        accounts.push_back(hardware_account.MakeAccountInfo());
      }
    }

    return accounts;
  }

  if (IsZCashKeyring(keyring_id)) {
    return accounts;
  }

  const base::Value::Dict* keyring =
      GetPrefForKeyringDict(profile_prefs_, kHardwareAccounts, keyring_id);
  if (!keyring) {
    return accounts;
  }

  for (auto&& [id, value] : *keyring) {
    DCHECK(value.is_dict());
    const base::Value* account_value = value.GetDict().Find(kAccountMetas);
    if (!account_value) {
      continue;
    }
    SerializeHardwareAccounts(id, account_value, keyring_id, &accounts);
  }

  return accounts;
}

void KeyringService::AddHardwareAccounts(
    std::vector<mojom::HardwareWalletAccountPtr> infos,
    AddHardwareAccountsCallback callback) {
  return std::move(callback).Run(AddHardwareAccountsSync(std::move(infos)));
}

std::vector<mojom::AccountInfoPtr> KeyringService::AddHardwareAccountsSync(
    std::vector<mojom::HardwareWalletAccountPtr> infos) {
  if (!IsWalletCreatedSync() || IsLockedSync()) {
    return {};
  }

  if (infos.empty()) {
    return {};
  }

  ScopedDictPrefUpdate keyrings_update(profile_prefs_, kBraveWalletKeyrings);

  std::vector<mojom::AccountInfoPtr> accounts_added;
  for (const auto& info : infos) {
    mojom::KeyringId keyring_id = info->keyring_id;
    DCHECK(info->hardware_vendor == mojom::HardwareVendor::kLedger ||
           info->hardware_vendor == mojom::HardwareVendor::kTrezor);
    std::string hardware_vendor_string =
        info->hardware_vendor == mojom::HardwareVendor::kLedger
            ? kLedgerPrefValue
            : kTrezorPrefValue;
    const std::string& device_id = info->device_id;

    base::Value::Dict hw_account;
    hw_account.Set(kAccountName, info->name);
    hw_account.Set(kHardwareVendor, hardware_vendor_string);
    hw_account.Set(kHardwareDerivationPath, info->derivation_path);

    base::Value::Dict& hardware_keyrings = GetDictPrefForKeyringUpdate(
        keyrings_update, kHardwareAccounts, info->keyring_id);

    hardware_keyrings.EnsureDict(device_id)
        ->EnsureDict(kAccountMetas)
        ->Set(info->address, std::move(hw_account));

    auto account_info = MakeAccountInfoForHardwareAccount(
        info->address, info->name, info->derivation_path, info->hardware_vendor,
        info->device_id, keyring_id);

    accounts_added.push_back(std::move(account_info));
  }
  NotifyAccountsChanged();

  // TODO(apaymyshev): ui should select account after importing.
  SetSelectedAccountInternal(*accounts_added.front());
  NotifyAccountsAdded(accounts_added);

  return accounts_added;
}

void KeyringService::AddBitcoinHardwareAccount(
    mojom::HardwareWalletAccountPtr info,
    AddBitcoinHardwareAccountCallback callback) {
  return std::move(callback).Run(
      !!AddBitcoinHardwareAccountSync(std::move(info)));
}

mojom::AccountInfoPtr KeyringService::AddBitcoinHardwareAccountSync(
    mojom::HardwareWalletAccountPtr info) {
  if (!IsWalletCreatedSync() || IsLockedSync()) {
    return nullptr;
  }

  std::vector<HardwareAccountInfo> hw_account_infos;
  auto* keyring = GetKeyring<BitcoinHardwareKeyring>(info->keyring_id);
  if (!keyring) {
    return nullptr;
  }

  uint32_t account_index =
      GenerateNextAccountIndex(profile_prefs_, info->keyring_id);

  if (!keyring->AddAccount(account_index, info->address)) {
    return nullptr;
  }

  HardwareAccountInfo hardware_account_info(
      info->keyring_id, account_index, info->name, info->hardware_vendor,
      info->derivation_path, info->device_id);
  hardware_account_info.bitcoin_xpub = info->address;
  hardware_account_info.bitcoin_next_change_address_index = 0;
  hardware_account_info.bitcoin_next_receive_address_index = 0;
  AddHardwareAccountToPrefs(profile_prefs_, hardware_account_info);

  NotifyAccountsChanged();
  auto account_info = hardware_account_info.MakeAccountInfo();
  NotifyAccountsAdded(*account_info);

  return account_info;
}

bool KeyringService::RemoveHardwareAccountInternal(
    const mojom::AccountId& account_id) {
  if (IsBitcoinHardwareKeyring(account_id.keyring_id)) {
    if (auto* keyring =
            GetKeyring<BitcoinHardwareKeyring>(account_id.keyring_id)) {
      if (!keyring->RemoveAccount(account_id.account_index)) {
        return false;
      }
    }

    RemoveHardwareAccountFromPrefs(profile_prefs_, account_id);

    NotifyAccountsChanged();
    return true;
  }

  ScopedDictPrefUpdate keyrings_update(profile_prefs_, kBraveWalletKeyrings);
  base::Value::Dict& hardware_keyrings = GetDictPrefForKeyringUpdate(
      keyrings_update, kHardwareAccounts, account_id.keyring_id);
  for (auto&& [id, device] : hardware_keyrings) {
    DCHECK(device.is_dict());
    base::Value::Dict* account_metas = device.GetDict().FindDict(kAccountMetas);
    if (!account_metas) {
      continue;
    }
    const base::Value* address_key = account_metas->Find(account_id.address);
    if (!address_key) {
      continue;
    }
    account_metas->Remove(account_id.address);

    if (account_metas->empty()) {
      hardware_keyrings.Remove(id);
    }
    NotifyAccountsChanged();

    MaybeFixAccountSelection();
    return true;
  }

  return false;
}

std::optional<std::string> KeyringService::SignTransactionByFilecoinKeyring(
    const mojom::AccountIdPtr& account_id,
    const FilTransaction& tx) {
  auto* keyring = GetKeyring<FilecoinKeyring>(account_id);
  if (!keyring) {
    return std::nullopt;
  }
  return keyring->SignTransaction(account_id->address, tx);
}

std::optional<std::string> KeyringService::GetDiscoveryAddress(
    mojom::KeyringId keyring_id,
    int index) {
  if (auto* keyring = GetKeyring<EthereumKeyring>(keyring_id)) {
    return keyring->GetDiscoveryAddress(index);
  }

  if (auto* keyring = GetKeyring<SolanaKeyring>(keyring_id)) {
    return keyring->GetDiscoveryAddress(index);
  }

  if (auto* keyring = GetKeyring<FilecoinKeyring>(keyring_id)) {
    return keyring->GetDiscoveryAddress(index);
  }

  return std::nullopt;
}

void KeyringService::SignTransactionByDefaultKeyring(
    const mojom::AccountIdPtr& account_id,
    EthTransaction* tx,
    uint256_t chain_id) {
  auto* keyring = GetKeyring<EthereumKeyring>(account_id);
  if (!keyring) {
    return;
  }
  keyring->SignTransaction(account_id->address, tx, chain_id);
}

base::expected<std::vector<uint8_t>, std::string>
KeyringService::SignMessageByDefaultKeyring(
    const mojom::AccountIdPtr& account_id,
    base::span<const uint8_t> message,
    bool is_eip712) {
  auto* keyring = GetKeyring<EthereumKeyring>(account_id);
  if (!keyring) {
    return base::unexpected(
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SIGN_MESSAGE_UNLOCK_FIRST));
  }

  auto address = account_id->address;
  // MM currently doesn't provide chain_id when signing message
  auto signature = keyring->SignMessage(address, message, 0, is_eip712);
  if (!signature) {
    return base::unexpected(
        l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_SIGN_MESSAGE_INVALID_ADDRESS,
                                  base::ASCIIToUTF16(address)));
  }
  return base::ok(*signature);
}

std::optional<std::string> KeyringService::RecoverAddressByDefaultKeyring(
    base::span<const uint8_t> message,
    base::span<const uint8_t> eth_signature) {
  return EthereumKeyring::RecoverAddress(message, eth_signature);
}

bool KeyringService::GetPublicKeyFromX25519_XSalsa20_Poly1305ByDefaultKeyring(
    const mojom::AccountIdPtr& account_id,
    std::string* key) {
  CHECK(account_id);
  CHECK(key);
  auto* keyring = GetKeyring<EthereumKeyring>(account_id);
  if (!keyring) {
    return false;
  }
  return keyring->GetPublicKeyFromX25519_XSalsa20_Poly1305(
      EthAddress::FromHex(account_id->address).ToChecksumAddress(), key);
}

std::optional<std::vector<uint8_t>>
KeyringService::DecryptCipherFromX25519_XSalsa20_Poly1305ByDefaultKeyring(
    const mojom::AccountIdPtr& account_id,
    const std::string& version,
    base::span<const uint8_t> nonce,
    base::span<const uint8_t> ephemeral_public_key,
    base::span<const uint8_t> ciphertext) {
  CHECK(account_id);
  auto* keyring = GetKeyring<EthereumKeyring>(account_id);
  if (!keyring) {
    return std::nullopt;
  }

  return keyring->DecryptCipherFromX25519_XSalsa20_Poly1305(
      version, nonce, ephemeral_public_key, ciphertext, account_id->address);
}

std::vector<uint8_t> KeyringService::SignMessageBySolanaKeyring(
    const mojom::AccountIdPtr& account_id,
    base::span<const uint8_t> message) {
  auto* keyring = GetKeyring<SolanaKeyring>(account_id);
  if (!keyring) {
    return {};
  }

  return keyring->SignMessage(account_id->address, message);
}

void KeyringService::AddAccountsWithDefaultName(
    const mojom::CoinType& coin_type,
    mojom::KeyringId keyring_id,
    size_t number) {
  std::string prefix;
  if (coin_type == mojom::CoinType::FIL) {
    prefix = "Filecoin ";
  } else if (coin_type == mojom::CoinType::SOL) {
    prefix = "Solana ";
  }

  std::vector<mojom::AccountInfoPtr> account_infos;
  size_t current_num =
      GetDerivedAccountsNumberForKeyring(profile_prefs_, keyring_id);
  for (size_t i = current_num + 1; i <= current_num + number; ++i) {
    auto add_result = AddHDAccountForKeyring(
        keyring_id, base::StrCat({prefix, GetAccountName(i)}));
    if (add_result) {
      account_infos.push_back(std::move(add_result));
    }
  }
  NotifyAccountsChanged();
  NotifyAccountsAdded(account_infos);
}

bool KeyringService::IsLockedSync() {
  if (!IsWalletCreatedSync()) {
    return false;
  }

  return !encryptor_;
}

bool KeyringService::HasPendingUnlockRequest() const {
  return request_unlock_pending_;
}

void KeyringService::Lock() {
  if (IsLockedSync()) {
    return;
  }

  ClearKeyrings();
  encryptor_.reset();

  for (const auto& observer : observers_) {
    observer->Locked();
  }
  StopAutoLockTimer();
}

void KeyringService::Unlock(const std::string& password,
                            KeyringService::UnlockCallback callback) {
  MaybeRunPasswordMigrations(profile_prefs_, password);

  auto encryptor = PasswordEncryptor::CreateEncryptor(
      password, GetSaltFromPrefs(profile_prefs_));
  if (!encryptor) {
    std::move(callback).Run(false);
    return;
  }

  auto mnemonic = DecryptWalletMnemonicFromPrefs(profile_prefs_, *encryptor);
  if (!mnemonic) {
    std::move(callback).Run(false);
    return;
  }

  auto keyring_seed =
      MakeSeedFromMnemonic(*mnemonic, IsLegacyEthSeedFormat(profile_prefs_));
  if (!keyring_seed) {
    std::move(callback).Run(false);
    return;
  }

  encryptor_ = std::move(encryptor);

  CreateKeyrings(*keyring_seed);
  LoadAllAccountsFromPrefs();

  UpdateLastUnlockPref(local_state_);
  request_unlock_pending_ = false;
  for (const auto& observer : observers_) {
    observer->Unlocked();
  }
  ResetAutoLockTimer();

  std::move(callback).Run(true);
}

void KeyringService::OnAutoLockFired() {
  Lock();
}

void KeyringService::IsLocked(IsLockedCallback callback) {
  std::move(callback).Run(IsLockedSync());
}

void KeyringService::Reset(bool notify_observer) {
  ResetAllAccountInfosCache();
  StopAutoLockTimer();
  encryptor_.reset();
  ClearKeyrings();
  ClearKeyringServiceProfilePrefs(profile_prefs_);
  if (notify_observer) {
    for (const auto& observer : observers_) {
      observer->WalletReset();
    }
  }
}

void KeyringService::StopAutoLockTimer() {
  auto_lock_timer_->Stop();
}

void KeyringService::ResetAutoLockTimer() {
  if (auto_lock_timer_->IsRunning()) {
    auto_lock_timer_->Reset();
  } else {
    size_t auto_lock_minutes =
        (size_t)profile_prefs_->GetInteger(kBraveWalletAutoLockMinutes);
    auto_lock_timer_->Start(FROM_HERE, base::Minutes(auto_lock_minutes), this,
                            &KeyringService::OnAutoLockFired);
  }
}

void KeyringService::AddObserver(
    ::mojo::PendingRemote<mojom::KeyringServiceObserver> observer) {
  observers_.Add(std::move(observer));
}

void KeyringService::NotifyUserInteraction() {
  if (auto_lock_timer_->IsRunning()) {
    auto_lock_timer_->Reset();
  }
}

void KeyringService::GetAllAccounts(GetAllAccountsCallback callback) {
  std::move(callback).Run(GetAllAccountsSync());
}

mojom::AllAccountsInfoPtr KeyringService::GetAllAccountsSync() {
  std::vector<mojom::AccountInfoPtr> all_accounts;
  for (const auto& account : GetAllAccountInfos()) {
    all_accounts.push_back(account.Clone());
  }

  return mojom::AllAccountsInfo::New(
      std::move(all_accounts), GetSelectedWalletAccount(),
      GetSelectedEthereumDappAccount(), GetSelectedSolanaDappAccount(),
      GetSelectedCardanoDappAccount());
}

void KeyringService::SetSelectedAccount(mojom::AccountIdPtr account_id,
                                        SetSelectedAccountCallback callback) {
  std::move(callback).Run(SetSelectedAccountSync(std::move(account_id)));
}

bool KeyringService::SetSelectedAccountSync(mojom::AccountIdPtr account_id) {
  std::vector<mojom::AccountInfoPtr> infos =
      GetAccountInfosForKeyring(account_id->keyring_id);

  for (const mojom::AccountInfoPtr& info : infos) {
    if (info->account_id == account_id) {
      SetSelectedAccountInternal(*info);
      return true;
    }
  }

  return false;
}

void KeyringService::SetAccountName(mojom::AccountIdPtr account_id,
                                    const std::string& name,
                                    SetAccountNameCallback callback) {
  if (name.empty()) {
    std::move(callback).Run(false);
    return;
  }

  switch (account_id->kind) {
    case mojom::AccountKind::kDerived:
      std::move(callback).Run(
          SetKeyringDerivedAccountNameInternal(*account_id, name));
      return;
    case mojom::AccountKind::kImported:
      std::move(callback).Run(
          SetKeyringImportedAccountNameInternal(*account_id, name));
      return;
    case mojom::AccountKind::kHardware:
      std::move(callback).Run(
          SetHardwareAccountNameInternal(*account_id, name));
      return;
  }
  NOTREACHED() << account_id->kind;
}

bool KeyringService::SetKeyringDerivedAccountNameInternal(
    const mojom::AccountId& account_id,
    const std::string& name) {
  DCHECK(!name.empty());

  const auto keyring_id = account_id.keyring_id;

  auto accounts = GetDerivedAccountsForKeyring(profile_prefs_, keyring_id);
  for (auto& account : accounts) {
    if (account_id == *account.GetAccountId()) {
      account.account_name = name;
      SetDerivedAccountsForKeyring(profile_prefs_, keyring_id, accounts);
      NotifyAccountsChanged();
      return true;
    }
  }

  return false;
}

bool KeyringService::SetHardwareAccountNameInternal(
    const mojom::AccountId& account_id,
    const std::string& name) {
  DCHECK(!name.empty());

  if (IsBitcoinHardwareKeyring(account_id.keyring_id)) {
    auto accounts =
        GetHardwareAccountsForKeyring(profile_prefs_, account_id.keyring_id);
    for (auto& account : accounts) {
      if (account_id == *account.GetAccountId()) {
        account.account_name = name;
        SetHardwareAccountsForKeyring(profile_prefs_, account_id.keyring_id,
                                      accounts);
        NotifyAccountsChanged();
        return true;
      }
    }
    return false;
  }

  ScopedDictPrefUpdate keyrings_update(profile_prefs_, kBraveWalletKeyrings);
  base::Value::Dict& hardware_keyrings = GetDictPrefForKeyringUpdate(
      keyrings_update, kHardwareAccounts, account_id.keyring_id);
  for (auto&& [id, device] : hardware_keyrings) {
    DCHECK(device.is_dict());
    base::Value::Dict* account_metas = device.GetDict().FindDict(kAccountMetas);
    if (!account_metas) {
      continue;
    }
    base::Value::Dict* address_key =
        account_metas->FindDict(account_id.address);
    if (!address_key) {
      continue;
    }
    address_key->Set(kAccountName, name);
    NotifyAccountsChanged();
    return true;
  }
  return false;
}

bool KeyringService::SetKeyringImportedAccountNameInternal(
    const mojom::AccountId& account_id,
    const std::string& name) {
  DCHECK(!name.empty());

  const auto keyring_id = account_id.keyring_id;

  auto accounts = GetImportedAccountsForKeyring(profile_prefs_, keyring_id);
  for (auto& account : accounts) {
    if (account_id == *account.GetAccountId()) {
      account.account_name = name;
      SetImportedAccountsForKeyring(profile_prefs_, keyring_id, accounts);
      NotifyAccountsChanged();
      return true;
    }
  }

  return false;
}

void KeyringService::NotifyAccountsChanged() {
  ResetAllAccountInfosCache();
  for (const auto& observer : observers_) {
    observer->AccountsChanged();
  }
}

void KeyringService::NotifyAccountsAdded(
    const mojom::AccountInfo& added_account) {
  std::vector<mojom::AccountInfoPtr> added_accounts;
  added_accounts.push_back(added_account.Clone());
  NotifyAccountsAdded(added_accounts);
}

void KeyringService::NotifyAccountsAdded(
    const std::vector<mojom::AccountInfoPtr>& added_accounts) {
  for (const auto& observer : observers_) {
    std::vector<mojom::AccountInfoPtr> added_accounts_clone;
    for (auto& account : added_accounts) {
      added_accounts_clone.push_back(account->Clone());
    }
    observer->AccountsAdded(std::move(added_accounts_clone));
  }
}

void KeyringService::OnAutoLockPreferenceChanged() {
  StopAutoLockTimer();
  ResetAutoLockTimer();
  for (const auto& observer : observers_) {
    observer->AutoLockMinutesChanged();
  }
}

void KeyringService::NotifySelectedWalletAccountChanged(
    const mojom::AccountInfo& account) {
  for (const auto& observer : observers_) {
    observer->SelectedWalletAccountChanged(account.Clone());
  }
}

void KeyringService::NotifySelectedDappAccountChanged(
    mojom::CoinType coin,
    const mojom::AccountInfoPtr& account) {
  CHECK(CoinSupportsDapps(coin));

  for (const auto& observer : observers_) {
    observer->SelectedDappAccountChanged(coin, account.Clone());
  }
}

void KeyringService::GetAutoLockMinutes(GetAutoLockMinutesCallback callback) {
  std::move(callback).Run(
      profile_prefs_->GetInteger(kBraveWalletAutoLockMinutes));
}

void KeyringService::SetAutoLockMinutes(int32_t minutes,
                                        SetAutoLockMinutesCallback callback) {
  // Check bounds
  if (minutes < kAutoLockMinutesMin || minutes > kAutoLockMinutesMax) {
    std::move(callback).Run(false);
    return;
  }

  int32_t old_auto_lock_minutes =
      profile_prefs_->GetInteger(kBraveWalletAutoLockMinutes);
  if (minutes != old_auto_lock_minutes) {
    profile_prefs_->SetInteger(kBraveWalletAutoLockMinutes, minutes);
  }
  std::move(callback).Run(true);
}

void KeyringService::IsStrongPassword(const std::string& password,
                                      IsStrongPasswordCallback callback) {
  std::move(callback).Run(password.length() > 7);
}

bool KeyringService::ValidatePasswordInternal(const std::string& password) {
  return !!GetWalletMnemonicInternal(password);
}

std::optional<std::string> KeyringService::GetWalletMnemonicInternal(
    const std::string& password) {
  auto encryptor = PasswordEncryptor::CreateEncryptor(
      password, GetSaltFromPrefs(profile_prefs_));
  if (!encryptor) {
    return std::nullopt;
  }

  return DecryptWalletMnemonicFromPrefs(profile_prefs_, *encryptor);
}

void KeyringService::ValidatePassword(const std::string& password,
                                      ValidatePasswordCallback callback) {
  MaybeRunPasswordMigrations(profile_prefs_, password);

  std::move(callback).Run(ValidatePasswordInternal(password));
}

void KeyringService::GetChecksumEthAddress(
    const std::string& address,
    GetChecksumEthAddressCallback callback) {
  std::move(callback).Run(EthAddress::FromHex(address).ToChecksumAddress());
}

void KeyringService::HasPendingUnlockRequest(
    HasPendingUnlockRequestCallback callback) {
  std::move(callback).Run(HasPendingUnlockRequest());
}

std::optional<std::vector<mojom::ZCashAddressPtr>>
KeyringService::GetZCashAddresses(const mojom::AccountIdPtr& account_id) {
  CHECK(IsZCashAccount(account_id));

  auto* zcash_keyring = GetKeyring<ZCashKeyring>(account_id->keyring_id);
  if (!zcash_keyring) {
    return std::nullopt;
  }

  auto zcash_account_info = GetZCashAccountInfo(account_id);
  if (!zcash_account_info) {
    return std::nullopt;
  }

  std::vector<mojom::ZCashAddressPtr> addresses;
  for (auto i = 0u;
       i < zcash_account_info->next_transparent_receive_address->key_id->index;
       ++i) {
    auto key_id = mojom::ZCashKeyId::New(account_id->account_index,
                                         kBitcoinReceiveIndex, i);
    auto address = zcash_keyring->GetTransparentAddress(*key_id);
    if (!address) {
      return std::nullopt;
    }
    addresses.emplace_back(std::move(address));
  }
  for (auto i = 0u;
       i < zcash_account_info->next_transparent_change_address->key_id->index;
       ++i) {
    auto key_id = mojom::ZCashKeyId::New(account_id->account_index,
                                         kBitcoinChangeIndex, i);
    auto address = zcash_keyring->GetTransparentAddress(*key_id);
    if (!address) {
      return std::nullopt;
    }
    addresses.emplace_back(std::move(address));
  }

  return addresses;
}

mojom::ZCashAddressPtr KeyringService::GetZCashAddress(
    const mojom::AccountIdPtr& account_id,
    const mojom::ZCashKeyId& key_id) {
  CHECK(IsZCashAccount(account_id));
  CHECK_EQ(account_id->account_index, key_id.account);

  auto* zcash_keyring = GetKeyring<ZCashKeyring>(account_id->keyring_id);
  if (!zcash_keyring) {
    return nullptr;
  }

  return zcash_keyring->GetTransparentAddress(key_id);
}

std::optional<std::vector<uint8_t>> KeyringService::GetZCashPubKey(
    const mojom::AccountIdPtr& account_id,
    const mojom::ZCashKeyIdPtr& key_id) {
  CHECK(key_id);
  CHECK(IsZCashAccount(account_id));

  auto* zcash_keyring = GetKeyring<ZCashKeyring>(account_id->keyring_id);
  if (!zcash_keyring) {
    return std::nullopt;
  }

  return zcash_keyring->GetPubkey(*key_id);
}

#if BUILDFLAG(ENABLE_ORCHARD)
std::optional<OrchardAddrRawPart> KeyringService::GetOrchardRawBytes(
    const mojom::AccountIdPtr& account_id,
    const mojom::ZCashKeyIdPtr& key_id) {
  CHECK(key_id);
  CHECK(IsZCashAccount(account_id));

  auto* zcash_keyring = GetKeyring<ZCashKeyring>(account_id->keyring_id);
  if (!zcash_keyring) {
    return std::nullopt;
  }

  return zcash_keyring->GetOrchardRawBytes(*key_id);
}

std::optional<OrchardFullViewKey> KeyringService::GetOrchardFullViewKey(
    const mojom::AccountIdPtr& account_id) {
  auto* zcash_keyring = GetKeyring<ZCashKeyring>(account_id->keyring_id);
  if (!zcash_keyring) {
    return std::nullopt;
  }

  return zcash_keyring->GetOrchardFullViewKey(account_id->account_index);
}

std::optional<OrchardSpendingKey> KeyringService::GetOrchardSpendingKey(
    const mojom::AccountIdPtr& account_id) {
  auto* zcash_keyring = GetKeyring<ZCashKeyring>(account_id->keyring_id);
  if (!zcash_keyring) {
    return std::nullopt;
  }

  return zcash_keyring->GetOrchardSpendingKey(account_id->account_index);
}

#endif

void KeyringService::UpdateNextUnusedAddressForCardanoAccount(
    const mojom::AccountIdPtr& account_id,
    std::optional<uint32_t> next_external_index,
    std::optional<uint32_t> next_internal_index) {
  CHECK(IsCardanoAccount(account_id));
  CHECK(next_external_index || next_internal_index);

  const auto keyring_id = account_id->keyring_id;

  if (IsCardanoHDKeyring(keyring_id)) {
    auto accounts = GetDerivedAccountsForKeyring(profile_prefs_, keyring_id);
    for (auto& account : accounts) {
      if (account_id == account.GetAccountId()) {
        if (UpdateCardanoAccountIndexes(account, next_external_index,
                                        next_internal_index)) {
          SetDerivedAccountsForKeyring(profile_prefs_, keyring_id, accounts);
          NotifyAccountsChanged();
          return;
        }
      }
    }
    return;
  }
}

mojom::CardanoAccountInfoPtr KeyringService::GetCardanoAccountInfo(
    const mojom::AccountIdPtr& account_id) {
  CHECK(IsCardanoAccount(account_id));

  auto keyring_id = account_id->keyring_id;
  if (auto* cardano_keyring = GetKeyring<CardanoHDKeyring>(keyring_id)) {
    for (const auto& derived_account_info :
         GetDerivedAccountsForKeyring(profile_prefs_, keyring_id)) {
      if (account_id->account_index == derived_account_info.account_index) {
        return CardanoAccountInfoFromPrefInfo(*cardano_keyring,
                                              derived_account_info);
      }
    }
  }

  return nullptr;
}

std::optional<std::vector<mojom::CardanoAddressPtr>>
KeyringService::GetCardanoAddresses(const mojom::AccountIdPtr& account_id) {
  CHECK(IsCardanoAccount(account_id));

  auto* cardano_keyring = GetKeyring<CardanoHDKeyring>(account_id->keyring_id);
  if (!cardano_keyring) {
    return std::nullopt;
  }

  auto cardano_account_info = GetCardanoAccountInfo(account_id);
  if (!cardano_account_info) {
    return std::nullopt;
  }

  std::vector<mojom::CardanoAddressPtr> addresses;
  for (auto i = 0u;
       i <= cardano_account_info->next_external_address->payment_key_id->index;
       ++i) {
    addresses.emplace_back(cardano_keyring->GetAddress(
        account_id->account_index,
        mojom::CardanoKeyId(mojom::CardanoKeyRole::kExternal, i)));
    if (!addresses.back()) {
      return std::nullopt;
    }
  }
  for (auto i = 0u;
       i <= cardano_account_info->next_internal_address->payment_key_id->index;
       ++i) {
    addresses.emplace_back(cardano_keyring->GetAddress(
        account_id->account_index,
        mojom::CardanoKeyId(mojom::CardanoKeyRole::kInternal, i)));
    if (!addresses.back()) {
      return std::nullopt;
    }
  }

  return addresses;
}

mojom::CardanoAddressPtr KeyringService::GetCardanoAddress(
    const mojom::AccountIdPtr& account_id,
    const mojom::CardanoKeyIdPtr& payment_key_id) {
  CHECK(IsCardanoAccount(account_id));
  CHECK(payment_key_id);

  auto* cardano_keyring = GetKeyring<CardanoHDKeyring>(account_id->keyring_id);
  if (!cardano_keyring) {
    return {};
  }

  return cardano_keyring->GetAddress(account_id->account_index,
                                     *payment_key_id);
}

std::optional<CardanoSignMessageResult>
KeyringService::SignMessageByCardanoKeyring(
    const mojom::AccountIdPtr& account_id,
    const mojom::CardanoKeyIdPtr& key_id,
    base::span<const uint8_t> message) {
  CHECK(IsCardanoAccount(account_id));
  CHECK(key_id);

  auto* cardano_keyring = GetKeyring<CardanoHDKeyring>(account_id->keyring_id);
  if (!cardano_keyring) {
    return std::nullopt;
  }

  return cardano_keyring->SignMessage(account_id->account_index, *key_id,
                                      message);
}

std::optional<base::Value::Dict>
KeyringService::SignCip30MessageByCardanoKeyring(
    const mojom::AccountIdPtr& account_id,
    const mojom::CardanoKeyIdPtr& key_id,
    base::span<const uint8_t> message) {
  CHECK(IsCardanoAccount(account_id));
  CHECK(key_id);

  auto* cardano_keyring = GetKeyring<CardanoHDKeyring>(account_id->keyring_id);
  if (!cardano_keyring) {
    return std::nullopt;
  }

  auto address =
      cardano_keyring->GetAddress(account_id->account_index, *key_id);
  if (!address) {
    return std::nullopt;
  }

  auto cardano_address = CardanoAddress::FromString(address->address_string);
  if (!cardano_address) {
    return std::nullopt;
  }

  auto signature_pair = cardano_keyring->SignMessage(
      account_id->account_index, *key_id,
      CardanoCip30Serializer::SerializedSignPayload(*cardano_address, message));
  if (!signature_pair) {
    return std::nullopt;
  }

  base::Value::Dict result;
  result.Set("key",
             HexEncodeLower(CardanoCip30Serializer::SerializeSignedDataKey(
                 *cardano_address, signature_pair->pubkey)));
  result.Set(
      "signature",
      HexEncodeLower(CardanoCip30Serializer::SerializeSignedDataSignature(
          *cardano_address, message, signature_pair->signature)));
  return result;
}

void KeyringService::UpdateNextUnusedAddressForBitcoinAccount(
    const mojom::AccountIdPtr& account_id,
    std::optional<uint32_t> next_receive_index,
    std::optional<uint32_t> next_change_index) {
  CHECK(IsBitcoinAccount(account_id));
  CHECK(next_receive_index || next_change_index);

  const auto keyring_id = account_id->keyring_id;

  if (IsBitcoinHDKeyring(keyring_id)) {
    auto accounts = GetDerivedAccountsForKeyring(profile_prefs_, keyring_id);
    for (auto& account : accounts) {
      if (account_id == account.GetAccountId()) {
        if (UpdateBitcoinAccountIndexes(account, next_receive_index,
                                        next_change_index)) {
          SetDerivedAccountsForKeyring(profile_prefs_, keyring_id, accounts);
          NotifyAccountsChanged();
          return;
        }
      }
    }
    return;
  }

  if (IsBitcoinImportKeyring(keyring_id)) {
    auto accounts = GetImportedAccountsForKeyring(profile_prefs_, keyring_id);
    for (auto& account : accounts) {
      if (account_id == account.GetAccountId()) {
        if (UpdateBitcoinAccountIndexes(account, next_receive_index,
                                        next_change_index)) {
          SetImportedAccountsForKeyring(profile_prefs_, keyring_id, accounts);
          NotifyAccountsChanged();
          return;
        }
      }
    }
    return;
  }

  if (IsBitcoinHardwareKeyring(keyring_id)) {
    auto accounts = GetHardwareAccountsForKeyring(profile_prefs_, keyring_id);
    for (auto& account : accounts) {
      if (account_id == account.GetAccountId()) {
        if (UpdateBitcoinAccountIndexes(account, next_receive_index,
                                        next_change_index)) {
          SetHardwareAccountsForKeyring(profile_prefs_, keyring_id, accounts);
          NotifyAccountsChanged();
          return;
        }
      }
    }
    return;
  }

  NOTREACHED() << keyring_id;
}

bool KeyringService::SetZCashAccountBirthday(
    const mojom::AccountIdPtr& account_id,
    mojom::ZCashAccountShieldBirthdayPtr account_birthday) {
  CHECK(IsZCashAccount(account_id));

  ScopedDictPrefUpdate keyrings_update(profile_prefs_, kBraveWalletKeyrings);
  base::Value::List& account_metas = GetListPrefForKeyringUpdate(
      keyrings_update, kAccountMetas, account_id->keyring_id);
  for (auto& item : account_metas) {
    if (auto derived_account =
            DerivedAccountInfo::FromValue(account_id->keyring_id, item)) {
      if (account_id ==
          MakeAccountInfoForDerivedAccount(*derived_account)->account_id) {
        derived_account->zcash_account_birthday = {account_birthday->value,
                                                   account_birthday->hash};

        item = derived_account->ToValue();
        NotifyAccountsChanged();
        return true;
      }
    }
  }
  return false;
}

void KeyringService::UpdateNextUnusedAddressForZCashAccount(
    const mojom::AccountIdPtr& account_id,
    std::optional<uint32_t> next_receive_index,
    std::optional<uint32_t> next_change_index) {
  CHECK(IsZCashAccount(account_id));
  CHECK(next_receive_index || next_change_index);

  const auto keyring_id = account_id->keyring_id;

  auto accounts = GetDerivedAccountsForKeyring(profile_prefs_, keyring_id);
  for (auto& account : accounts) {
    if (account_id == account.GetAccountId()) {
      bool account_changed = false;
      if (next_receive_index) {
        account_changed =
            account_changed ||
            account.bitcoin_next_receive_address_index != *next_receive_index;
        account.bitcoin_next_receive_address_index = *next_receive_index;
      }
      if (next_change_index) {
        account_changed =
            account_changed ||
            account.bitcoin_next_change_address_index != *next_change_index;
        account.bitcoin_next_change_address_index = *next_change_index;
      }
      if (account_changed) {
        SetDerivedAccountsForKeyring(profile_prefs_, keyring_id, accounts);
        NotifyAccountsChanged();
        return;
      }
    }
  }
  return;
}

mojom::BitcoinAccountInfoPtr KeyringService::GetBitcoinAccountInfo(
    const mojom::AccountIdPtr& account_id) {
  CHECK(IsBitcoinAccount(account_id));

  auto keyring_id = account_id->keyring_id;
  if (auto* bitcoin_keyring = GetKeyring<BitcoinHDKeyring>(keyring_id)) {
    for (const auto& derived_account_info :
         GetDerivedAccountsForKeyring(profile_prefs_, keyring_id)) {
      if (account_id->account_index == derived_account_info.account_index) {
        return BitcoinAccountInfoFromPrefInfo(*bitcoin_keyring,
                                              derived_account_info);
      }
    }
  }

  if (auto* bitcoin_keyring = GetKeyring<BitcoinImportKeyring>(keyring_id)) {
    for (const auto& imported_account_info :
         GetImportedAccountsForKeyring(profile_prefs_, keyring_id)) {
      if (account_id->account_index == imported_account_info.account_index) {
        return BitcoinAccountInfoFromPrefInfo(*bitcoin_keyring,
                                              imported_account_info);
      }
    }
  }

  if (auto* bitcoin_keyring = GetKeyring<BitcoinHardwareKeyring>(keyring_id)) {
    for (const auto& hw_account_info :
         GetHardwareAccountsForKeyring(profile_prefs_, keyring_id)) {
      if (account_id->account_index == hw_account_info.account_index) {
        return BitcoinAccountInfoFromPrefInfo(*bitcoin_keyring,
                                              hw_account_info);
      }
    }
  }

  return nullptr;
}

std::optional<std::vector<mojom::BitcoinAddressPtr>>
KeyringService::GetBitcoinAddresses(const mojom::AccountIdPtr& account_id) {
  CHECK(IsBitcoinAccount(account_id));

  auto* bitcoin_keyring =
      GetKeyring<BitcoinBaseKeyring>(account_id->keyring_id);
  if (!bitcoin_keyring) {
    return std::nullopt;
  }

  auto bitcoin_account_info = GetBitcoinAccountInfo(account_id);
  if (!bitcoin_account_info) {
    return std::nullopt;
  }

  std::vector<mojom::BitcoinAddressPtr> addresses;
  for (auto i = 0u;
       i <= bitcoin_account_info->next_receive_address->key_id->index; ++i) {
    addresses.emplace_back(bitcoin_keyring->GetAddress(
        account_id->account_index,
        mojom::BitcoinKeyId(kBitcoinReceiveIndex, i)));
    if (!addresses.back()) {
      return std::nullopt;
    }
  }
  for (auto i = 0u;
       i <= bitcoin_account_info->next_change_address->key_id->index; ++i) {
    addresses.emplace_back(bitcoin_keyring->GetAddress(
        account_id->account_index,
        mojom::BitcoinKeyId(kBitcoinChangeIndex, i)));
    if (!addresses.back()) {
      return std::nullopt;
    }
  }

  return addresses;
}

mojom::BitcoinAddressPtr KeyringService::GetBitcoinAddress(
    const mojom::AccountIdPtr& account_id,
    const mojom::BitcoinKeyIdPtr& key_id) {
  CHECK(IsBitcoinAccount(account_id));
  CHECK(key_id);

  auto* bitcoin_keyring =
      GetKeyring<BitcoinBaseKeyring>(account_id->keyring_id);
  if (!bitcoin_keyring) {
    return {};
  }

  return bitcoin_keyring->GetAddress(account_id->account_index, *key_id);
}

mojom::BitcoinAddressPtr KeyringService::GetBitcoinAccountDiscoveryAddress(
    const mojom::KeyringId keyring_id,
    uint32_t account_index,
    const mojom::BitcoinKeyIdPtr& key_id) {
  CHECK(IsBitcoinKeyring(keyring_id));
  auto* bitcoin_keyring = GetKeyring<BitcoinBaseKeyring>(keyring_id);
  if (!bitcoin_keyring) {
    return {};
  }

  return bitcoin_keyring->GetAddress(account_index, *key_id);
}

std::optional<std::vector<uint8_t>> KeyringService::GetBitcoinPubkey(
    const mojom::AccountIdPtr& account_id,
    const mojom::BitcoinKeyIdPtr& key_id) {
  CHECK(IsBitcoinAccount(account_id));
  CHECK(key_id);

  auto* bitcoin_keyring =
      GetKeyring<BitcoinBaseKeyring>(account_id->keyring_id);
  if (!bitcoin_keyring) {
    return std::nullopt;
  }

  return bitcoin_keyring->GetPubkey(account_id->account_index, *key_id);
}

std::optional<std::vector<uint8_t>> KeyringService::SignMessageByBitcoinKeyring(
    const mojom::AccountIdPtr& account_id,
    const mojom::BitcoinKeyIdPtr& key_id,
    base::span<const uint8_t, 32> message) {
  CHECK(IsBitcoinAccount(account_id));
  CHECK(key_id);

  auto* bitcoin_keyring =
      GetKeyring<BitcoinBaseKeyring>(account_id->keyring_id);
  if (!bitcoin_keyring) {
    return std::nullopt;
  }

  return bitcoin_keyring->SignMessage(account_id->account_index, *key_id,
                                      message);
}

mojom::ZCashAccountInfoPtr KeyringService::GetZCashAccountInfo(
    const mojom::AccountIdPtr& account_id) {
  CHECK(IsZCashAccount(account_id));

  auto keyring_id = account_id->keyring_id;

  auto* zcash_keyring = GetKeyring<ZCashKeyring>(keyring_id);
  if (!zcash_keyring) {
    return {};
  }

  for (const auto& derived_account_info :
       GetDerivedAccountsForKeyring(profile_prefs_, keyring_id)) {
    if (account_id->account_index != derived_account_info.account_index) {
      continue;
    }
    auto result = mojom::ZCashAccountInfo::New();

    auto receive_key_id = mojom::ZCashKeyId::New(
        account_id->account_index, kBitcoinReceiveIndex,
        derived_account_info.bitcoin_next_receive_address_index.value_or(0));
    auto receive_address =
        zcash_keyring->GetTransparentAddress(*receive_key_id);
    if (!receive_address) {
      return {};
    }
    result->next_transparent_receive_address = std::move(receive_address);

    auto change_key_id = mojom::ZCashKeyId::New(
        account_id->account_index, kBitcoinChangeIndex,
        derived_account_info.bitcoin_next_change_address_index.value_or(0));
    auto change_address = zcash_keyring->GetTransparentAddress(*change_key_id);
    if (!change_address) {
      return {};
    }

    result->next_transparent_change_address = std::move(change_address);

    if (derived_account_info.zcash_account_birthday) {
      result->account_shield_birthday = mojom::ZCashAccountShieldBirthday::New(
          derived_account_info.zcash_account_birthday->first,
          derived_account_info.zcash_account_birthday->second);
    }

#if BUILDFLAG(ENABLE_ORCHARD)
    auto unified_address = zcash_keyring->GetUnifiedAddress(
        *receive_key_id,
        *mojom::ZCashKeyId::New(account_id->account_index, 0, 0));
    if (unified_address) {
      result->unified_address = *unified_address;
    }
    auto orchard_address = zcash_keyring->GetShieldedAddress(
        *mojom::ZCashKeyId::New(account_id->account_index, 0, 0));
    if (orchard_address) {
      result->orchard_address = orchard_address->address_string;
    }
    auto orchard_internal_address = zcash_keyring->GetShieldedAddress(
        *mojom::ZCashKeyId::New(account_id->account_index, 1, 0));
    if (orchard_internal_address) {
      result->orchard_internal_address =
          orchard_internal_address->address_string;
    }
#endif  // BUILDFLAG(ENABLE_ORCHARD)

    return result;
  }
  return {};
}

std::optional<std::vector<uint8_t>> KeyringService::SignMessageByZCashKeyring(
    const mojom::AccountIdPtr& account_id,
    const mojom::ZCashKeyIdPtr& key_id,
    const base::span<const uint8_t, 32> message) {
  CHECK(IsZCashAccount(account_id));
  CHECK(key_id);

  auto* zcash_keyring = GetKeyring<ZCashKeyring>(account_id->keyring_id);
  if (!zcash_keyring) {
    return std::nullopt;
  }

  return zcash_keyring->SignMessage(*key_id, message);
}

void KeyringService::ResetAllAccountInfosCache() {
  account_info_cache_.reset();
}

const std::vector<mojom::AccountInfoPtr>& KeyringService::GetAllAccountInfos() {
  if (!account_info_cache_ || account_info_cache_->empty()) {
    account_info_cache_ =
        std::make_unique<std::vector<mojom::AccountInfoPtr>>();
    for (const auto& keyring_id : GetEnabledKeyrings()) {
      for (auto& account_info : GetAccountInfosForKeyring(keyring_id)) {
        account_info_cache_->push_back(std::move(account_info));
      }
    }
  }
  return *account_info_cache_;
}

mojom::AccountInfoPtr KeyringService::FindAccount(
    const mojom::AccountIdPtr& account_id) {
  const auto& accounts = GetAllAccountInfos();
  for (auto& account : accounts) {
    if (account->account_id == account_id) {
      return account.Clone();
    }
  }
  return nullptr;
}

mojom::AccountInfoPtr KeyringService::GetSelectedWalletAccount() {
  const auto& account_infos = GetAllAccountInfos();
  auto unique_key =
      profile_prefs_->GetString(kBraveWalletSelectedWalletAccount);

  mojom::AccountInfoPtr first_account;
  for (auto& account_info : account_infos) {
    if (account_info->account_id->unique_key == unique_key) {
      return account_info->Clone();
    }
    if (!first_account) {
      first_account = account_info->Clone();
    }
  }
  return first_account;
}

mojom::AccountInfoPtr KeyringService::GetSelectedEthereumDappAccount() {
  return GetSelectedDappAccount(mojom::CoinType::ETH);
}

mojom::AccountInfoPtr KeyringService::GetSelectedSolanaDappAccount() {
  return GetSelectedDappAccount(mojom::CoinType::SOL);
}

mojom::AccountInfoPtr KeyringService::GetSelectedCardanoDappAccount() {
  return GetSelectedDappAccount(mojom::CoinType::ADA);
}

mojom::AccountInfoPtr KeyringService::GetSelectedDappAccount(
    mojom::CoinType coin) {
  CHECK(CoinSupportsDapps(coin));

  mojom::KeyringId keyring_id = mojom::KeyringId::kDefault;
  switch (coin) {
    case mojom::CoinType::ETH:
      break;
    case mojom::CoinType::SOL:
      keyring_id = mojom::KeyringId::kSolana;
      break;
    case mojom::CoinType::ADA:
      keyring_id = mojom::KeyringId::kCardanoMainnet;
      break;
    default:
      NOTREACHED();
  }

  auto unique_key = GetSelectedDappAccountFromPrefs(profile_prefs_, coin);

  for (auto& account_info : GetAccountInfosForKeyring(keyring_id)) {
    if (account_info->account_id->unique_key == unique_key) {
      return account_info->Clone();
    }
  }

  return {};
}

void KeyringService::MaybeFixAccountSelection() {
  const auto& account_infos = GetAllAccountInfos();
  if (account_infos.empty()) {
    return;
  }

  std::set<std::string> unique_keys;
  for (auto& acc : account_infos) {
    unique_keys.insert(acc->account_id->unique_key);
  }

  if (!unique_keys.contains(
          GetSelectedWalletAccountFromPrefs(profile_prefs_))) {
    // Legacy behavior when after removing selected account ui picks first
    // available account.
    DCHECK_EQ(account_infos[0]->account_id->coin, mojom::CoinType::ETH);
    SetSelectedAccountInternal(*account_infos[0]);
  }

  if (!unique_keys.contains(GetSelectedDappAccountFromPrefs(
          profile_prefs_, mojom::CoinType::ETH))) {
    // ETH dApp account appears to be removed. Forcedly clear ETH dApp account
    // selection.
    SetSelectedDappAccountInternal(mojom::CoinType::ETH, {});
  }

  if (!unique_keys.contains(GetSelectedDappAccountFromPrefs(
          profile_prefs_, mojom::CoinType::SOL))) {
    // SOL dApp account appears to be removed. Forcedly clear SOL dApp account
    // selection.
    SetSelectedDappAccountInternal(mojom::CoinType::SOL, {});
  }
}

void KeyringService::MaybeUnlockWithCommandLine() {
#if !defined(OFFICIAL_BUILD)
  std::string dev_wallet_password =
      base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
          switches::kDevWalletPassword);
  if (!dev_wallet_password.empty()) {
    Unlock(dev_wallet_password, base::DoNothing());
  }
#endif  // !defined(OFFICIAL_BUILD)
}

}  // namespace brave_wallet
