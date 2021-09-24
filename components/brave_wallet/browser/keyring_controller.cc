/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/keyring_controller.h"

#include <utility>

#include "base/base64.h"
#include "base/hash/hash.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/value_iterators.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller.h"
#include "brave/components/brave_wallet/browser/hd_key.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "crypto/random.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

/* kBraveWalletKeyrings structure
 *
 * {
 *   "hardware":  {
 *     "Ledger12445": {
 *        "account_metas": {
 *            "0xEA04...CC8Acc": {
 *              "account_name": "Ledger 1",
 *              "derivation_path": "m/44'/60'/1'/0/0",
 *              "hardware_vendor": "ledger"
 *           },
 *           "0x264Ef...6b8F1": {
 *              "account_name": "Ledger 2",
 *              "derivation_path": "m/44'/60'/2'/0/0",
 *              "hardware_vendor": "ledger"
 *            }
 *        },
 *        device_name: "Ledger 123"
 *     },
 *     "Ledger44332":{
 *      ...
 *     }
 *   }
 * },
 * "default":
 *   {  "backup_complete": false,
 *      "encrypted_mnemonic": [mnemonic],
 *      "legacy_brave_wallet": false,
 *      "account_metas": {
 *         "m/44'/60'/0'/0/0": {
 *               "account_name": "account 1",
 *               ...
 *          },
 *          "m/44'/60'/0'/0/1": {
 *               "account_name": "account 2",
 *               ...
 *          }
 *      },
 *      "imported_accounts": [
 *        { "address": "0x71f430f5f2a79274c17986ea1a1106596a39ba05",
 *          "encrypted_private_key": [privatekey],
 *          "account_name": "Imported account 1"
 *        },
 *        ...
 *      ],
 *      ...
 *   },
 *
 *   [keyringid]: {...}
 *   ...
 * }
 */

namespace brave_wallet {
namespace {
const size_t kSaltSize = 32;
const size_t kNonceSize = 12;
const char kRootPath[] = "m/44'/60'/0'/0";
const char kDefaultKeyringId[] = "default";
// TODO(darkdh): use resource string
const char kFirstAccountName[] = "Account 1";
const char kPasswordEncryptorSalt[] = "password_encryptor_salt";
const char kPasswordEncryptorNonce[] = "password_encryptor_nonce";
const char kEncryptedMnemonic[] = "encrypted_mnemonic";
const char kBackupComplete[] = "backup_complete";
const char kAccountMetas[] = "account_metas";
const char kAccountName[] = "account_name";
const char kHardwareVendor[] = "hardware_vendor";
const char kImportedAccounts[] = "imported_accounts";
const char kAccountAddress[] = "account_address";
const char kEncryptedPrivateKey[] = "encrypted_private_key";
const char kLegacyBraveWallet[] = "legacy_brave_wallet";
const char kHardwareKeyrings[] = "hardware";
const char kHardwareDerivationPath[] = "derivation_path";

static base::span<const uint8_t> ToSpan(base::StringPiece sp) {
  return base::as_bytes(base::make_span(sp));
}

void SerializeHardwareAccounts(const base::Value* account_value,
                               std::vector<mojom::AccountInfoPtr>* accounts) {
  for (const auto account : account_value->DictItems()) {
    std::string address = account.first;
    std::string hardware_vendor;
    const std::string* hardware_value =
        account.second.FindStringKey(kHardwareVendor);
    if (hardware_value)
      hardware_vendor = *hardware_value;

    std::string name;
    const std::string* name_value = account.second.FindStringKey(kAccountName);
    if (name_value)
      name = *name_value;

    std::string derivation_path;
    const std::string* derivation_path_value =
        account.second.FindStringKey(kHardwareDerivationPath);
    if (derivation_path_value)
      derivation_path = *derivation_path_value;

    accounts->push_back(mojom::AccountInfo::New(
        address, name, false,
        mojom::HardwareInfo::New(derivation_path, hardware_vendor)));
  }
}

}  // namespace

KeyringController::KeyringController(PrefService* prefs) : prefs_(prefs) {
  DCHECK(prefs);
}

KeyringController::~KeyringController() {}

mojo::PendingRemote<mojom::KeyringController> KeyringController::MakeRemote() {
  mojo::PendingRemote<mojom::KeyringController> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void KeyringController::Bind(
    mojo::PendingReceiver<mojom::KeyringController> receiver) {
  receivers_.Add(this, std::move(receiver));
}

// static
void KeyringController::MigrateObsoleteProfilePrefs(PrefService* prefs) {
  if (prefs->HasPrefPath(kBraveWalletPasswordEncryptorSalt) &&
      prefs->HasPrefPath(kBraveWalletPasswordEncryptorNonce) &&
      prefs->HasPrefPath(kBraveWalletEncryptedMnemonic)) {
    SetPrefForKeyring(
        prefs, kPasswordEncryptorSalt,
        base::Value(prefs->GetString(kBraveWalletPasswordEncryptorSalt)),
        kDefaultKeyringId);
    SetPrefForKeyring(
        prefs, kPasswordEncryptorNonce,
        base::Value(prefs->GetString(kBraveWalletPasswordEncryptorNonce)),
        kDefaultKeyringId);
    SetPrefForKeyring(
        prefs, kEncryptedMnemonic,
        base::Value(prefs->GetString(kBraveWalletEncryptedMnemonic)),
        kDefaultKeyringId);
    prefs->ClearPref(kBraveWalletPasswordEncryptorSalt);
    prefs->ClearPref(kBraveWalletPasswordEncryptorNonce);
    prefs->ClearPref(kBraveWalletEncryptedMnemonic);
  }

  if (prefs->HasPrefPath(kBraveWalletDefaultKeyringAccountNum) &&
      prefs->HasPrefPath(kBraveWalletAccountNames)) {
    size_t account_num =
        (size_t)prefs->GetInteger(kBraveWalletDefaultKeyringAccountNum);
    const base::Value* account_names_list =
        prefs->GetList(kBraveWalletAccountNames);
    if (account_names_list &&
        account_names_list->GetList().size() == account_num) {
      base::Value::ConstListView account_names = account_names_list->GetList();
      for (size_t i = 0; i < account_names.size(); ++i) {
        SetAccountMetaForKeyring(prefs, GetAccountPathByIndex(i),
                                 account_names[i].GetString(), "",
                                 kDefaultKeyringId);
      }
    } else {
      // This shouldn't happen but we will reset account to default state as
      // fail-safe
      SetAccountMetaForKeyring(prefs, GetAccountPathByIndex(0),
                               kFirstAccountName, "", kDefaultKeyringId);
    }
    prefs->ClearPref(kBraveWalletDefaultKeyringAccountNum);
    prefs->ClearPref(kBraveWalletAccountNames);
  }

  if (prefs->HasPrefPath(kBraveWalletBackupComplete)) {
    SetPrefForKeyring(
        prefs, kBackupComplete,
        base::Value(prefs->GetBoolean(kBraveWalletBackupComplete)),
        kDefaultKeyringId);
    prefs->ClearPref(kBraveWalletBackupComplete);
  }
}

// static
bool KeyringController::HasPrefForKeyring(PrefService* prefs,
                                          const std::string& key,
                                          const std::string& id) {
  return GetPrefForKeyring(prefs, key, id) != nullptr;
}

// static
const base::Value* KeyringController::GetPrefForKeyring(PrefService* prefs,
                                                        const std::string& key,
                                                        const std::string& id) {
  DCHECK(prefs);
  const base::DictionaryValue* keyrings_pref =
      prefs->GetDictionary(kBraveWalletKeyrings);
  if (!keyrings_pref)
    return nullptr;
  const base::Value* keyring_dict = keyrings_pref->FindKey(id);
  if (!keyring_dict)
    return nullptr;

  return keyring_dict->FindKey(key);
}

// static
base::Value* KeyringController::GetPrefForKeyringUpdate(PrefService* prefs,
                                                        const std::string& key,
                                                        const std::string& id) {
  DCHECK(prefs);
  DictionaryPrefUpdate update(prefs, kBraveWalletKeyrings);
  base::DictionaryValue* keyrings_pref = update.Get();
  if (!keyrings_pref)
    return nullptr;
  base::Value* keyring_dict = keyrings_pref->FindKey(id);
  if (!keyring_dict)
    keyring_dict =
        keyrings_pref->SetKey(id, base::Value(base::Value::Type::DICTIONARY));
  base::Value* pref = keyring_dict->FindKey(key);
  if (!pref)
    pref =
        keyring_dict->SetKey(key, base::Value(base::Value::Type::DICTIONARY));
  return pref;
}

// static
void KeyringController::SetPrefForKeyring(PrefService* prefs,
                                          const std::string& key,
                                          base::Value value,
                                          const std::string& id) {
  DCHECK(prefs);
  DictionaryPrefUpdate update(prefs, kBraveWalletKeyrings);
  base::DictionaryValue* keyrings_pref = update.Get();

  if (!keyrings_pref->FindKey(id)) {
    keyrings_pref->SetKey(id, base::Value(base::Value::Type::DICTIONARY));
  }

  base::Value* keyring_dict = keyrings_pref->FindKey(id);
  if (!keyring_dict)
    return;

  keyring_dict->SetKey(key, std::move(value));
}

// static
void KeyringController::SetAccountMetaForKeyring(
    PrefService* prefs,
    const std::string& account_path,
    const absl::optional<std::string> name,
    const absl::optional<std::string> address,
    const std::string& id) {
  base::Value* account_metas =
      GetPrefForKeyringUpdate(prefs, kAccountMetas, id);
  if (!account_metas)
    return;

  if (!account_metas->FindKey(account_path))
    account_metas->SetKey(account_path,
                          base::Value(base::Value::Type::DICTIONARY));
  base::Value* account_meta = account_metas->FindKey(account_path);
  if (!account_meta)
    return;

  if (name)
    account_meta->SetStringKey(kAccountName, *name);
  if (address)
    account_meta->SetStringKey(kAccountAddress, *address);
}

// static
std::string KeyringController::GetAccountNameForKeyring(
    PrefService* prefs,
    const std::string& account_path,
    const std::string& id) {
  const base::Value* account_metas =
      GetPrefForKeyring(prefs, kAccountMetas, id);
  if (!account_metas)
    return std::string();

  const base::Value* name =
      account_metas->FindPath(account_path + "." + kAccountName);
  if (!name)
    return std::string();

  return name->GetString();
}

// static
std::string KeyringController::GetAccountAddressForKeyring(
    PrefService* prefs,
    const std::string& account_path,
    const std::string& id) {
  const base::Value* account_metas =
      GetPrefForKeyring(prefs, kAccountMetas, id);
  if (!account_metas)
    return std::string();

  const base::Value* address =
      account_metas->FindPath(account_path + "." + kAccountAddress);
  if (!address)
    return std::string();

  return address->GetString();
}

// static
std::string KeyringController::GetAccountPathByIndex(size_t index) {
  return std::string(kRootPath) + "/" + base::NumberToString(index);
}

// static
void KeyringController::SetImportedAccountForKeyring(
    PrefService* prefs,
    const ImportedAccountInfo& info,
    const std::string& id) {
  base::Value imported_account(base::Value::Type::DICTIONARY);
  imported_account.SetStringKey(kAccountName, info.account_name);
  imported_account.SetStringKey(kAccountAddress, info.account_address);
  imported_account.SetStringKey(kEncryptedPrivateKey,
                                info.encrypted_private_key);

  base::Value imported_accounts(base::Value::Type::LIST);
  const base::Value* value = GetPrefForKeyring(prefs, kImportedAccounts, id);
  if (value)
    imported_accounts = value->Clone();
  imported_accounts.Append(std::move(imported_account));

  SetPrefForKeyring(prefs, kImportedAccounts, std::move(imported_accounts), id);
}

// static
std::vector<KeyringController::ImportedAccountInfo>
KeyringController::GetImportedAccountsForKeyring(PrefService* prefs,
                                                 const std::string& id) {
  std::vector<ImportedAccountInfo> result;
  const base::Value* imported_accounts =
      GetPrefForKeyring(prefs, kImportedAccounts, id);
  if (!imported_accounts)
    return result;
  for (const auto& imported_account : imported_accounts->GetList()) {
    const std::string* account_name =
        imported_account.FindStringKey(kAccountName);
    const std::string* account_address =
        imported_account.FindStringKey(kAccountAddress);
    const std::string* encrypted_private_key =
        imported_account.FindStringKey(kEncryptedPrivateKey);
    if (!account_name || !account_address || !encrypted_private_key) {
      VLOG(0) << __func__ << "Imported accounts corruppted";
      continue;
    }
    result.push_back(ImportedAccountInfo(
        {*account_name, *account_address, *encrypted_private_key}));
  }
  return result;
}
// static
void KeyringController::RemoveImportedAccountForKeyring(
    PrefService* prefs,
    const std::string& address,
    const std::string& id) {
  base::Value imported_accounts(base::Value::Type::LIST);
  const base::Value* value = GetPrefForKeyring(prefs, kImportedAccounts, id);
  if (!value)
    return;
  imported_accounts = value->Clone();
  const auto imported_accounts_list = imported_accounts.GetList();
  for (const auto& imported_account : imported_accounts_list) {
    const std::string* account_address =
        imported_account.FindStringKey(kAccountAddress);
    if (account_address && *account_address == address) {
      imported_accounts.EraseListValue(imported_account);
      break;
    }
  }

  SetPrefForKeyring(prefs, kImportedAccounts, std::move(imported_accounts), id);
}

HDKeyring* KeyringController::CreateDefaultKeyring(
    const std::string& password) {
  if (!CreateEncryptorForKeyring(password, kDefaultKeyringId))
    return nullptr;

  const std::string mnemonic = GenerateMnemonic(16);
  if (!CreateDefaultKeyringInternal(mnemonic, false)) {
    return nullptr;
  }

  for (const auto& observer : observers_) {
    observer->KeyringCreated();
  }

  return default_keyring_.get();
}

HDKeyring* KeyringController::ResumeDefaultKeyring(
    const std::string& password) {
  if (!CreateEncryptorForKeyring(password, kDefaultKeyringId)) {
    return nullptr;
  }

  const std::string mnemonic = GetMnemonicForDefaultKeyringImpl();
  bool is_legacy_brave_wallet = false;
  const base::Value* value =
      GetPrefForKeyring(prefs_, kLegacyBraveWallet, kDefaultKeyringId);
  if (value)
    is_legacy_brave_wallet = value->GetBool();
  if (mnemonic.empty() ||
      !CreateDefaultKeyringInternal(mnemonic, is_legacy_brave_wallet)) {
    return nullptr;
  }
  size_t account_no = GetAccountMetasNumberForKeyring(kDefaultKeyringId);
  if (account_no)
    default_keyring_->AddAccounts(account_no);

  // TODO(bbondy):
  // We can remove this some months after the initial wallet launch
  // We didn't store account address in meta pref originally.
  for (size_t i = 0; i < account_no; ++i) {
    SetAccountMetaForKeyring(prefs_, GetAccountPathByIndex(i), absl::nullopt,
                             default_keyring_->GetAddress(i),
                             kDefaultKeyringId);
  }

  for (const auto& imported_account_info :
       GetImportedAccountsForKeyring(prefs_, kDefaultKeyringId)) {
    std::string private_key_decoded;
    if (!base::Base64Decode(imported_account_info.encrypted_private_key,
                            &private_key_decoded))
      continue;
    std::vector<uint8_t> private_key;
    if (!encryptor_->Decrypt(ToSpan(private_key_decoded),
                             GetOrCreateNonceForKeyring(kDefaultKeyringId),
                             &private_key)) {
      continue;
    }
    default_keyring_->ImportAccount(private_key);
  }

  return default_keyring_.get();
}

HDKeyring* KeyringController::RestoreDefaultKeyring(
    const std::string& mnemonic,
    const std::string& password,
    bool is_legacy_brave_wallet) {
  if (!IsValidMnemonic(mnemonic))
    return nullptr;

  // Try getting existing mnemonic first
  if (CreateEncryptorForKeyring(password, kDefaultKeyringId)) {
    const std::string current_mnemonic = GetMnemonicForDefaultKeyringImpl();
    // Restore with same mnmonic and same password, resume current keyring
    // Also need to make sure is_legacy_brave_wallet are the same, users might
    // choose the option wrongly and then want to start over with same mnemonic
    // but different is_legacy_brave_wallet value
    const base::Value* value =
        GetPrefForKeyring(prefs_, kLegacyBraveWallet, kDefaultKeyringId);
    if (!current_mnemonic.empty() && current_mnemonic == mnemonic && value &&
        value->GetBool() == is_legacy_brave_wallet) {
      return ResumeDefaultKeyring(password);
    } else {
      // We have no way to check if new mnemonic is same as current mnemonic so
      // we need to clear all prefs for fresh start
      Reset();
    }
  }

  if (!CreateEncryptorForKeyring(password, kDefaultKeyringId)) {
    return nullptr;
  }

  if (!CreateDefaultKeyringInternal(mnemonic, is_legacy_brave_wallet)) {
    return nullptr;
  }

  for (const auto& observer : observers_) {
    observer->KeyringRestored();
  }

  return default_keyring_.get();
}

void KeyringController::GetDefaultKeyringInfo(
    GetDefaultKeyringInfoCallback callback) {
  mojom::KeyringInfoPtr keyring_info = mojom::KeyringInfo::New();
  keyring_info->is_default_keyring_created = IsDefaultKeyringCreated();
  keyring_info->is_locked = IsLocked();
  bool backup_complete = false;
  const base::Value* value =
      GetPrefForKeyring(prefs_, kBackupComplete, kDefaultKeyringId);
  if (value)
    backup_complete = value->GetBool();
  keyring_info->is_backed_up = backup_complete;
  keyring_info->account_infos = GetAccountInfosForKeyring(kDefaultKeyringId);
  std::move(callback).Run(std::move(keyring_info));
}

void KeyringController::GetMnemonicForDefaultKeyring(
    GetMnemonicForDefaultKeyringCallback callback) {
  std::move(callback).Run(GetMnemonicForDefaultKeyringImpl());
}

void KeyringController::CreateWallet(const std::string& password,
                                     CreateWalletCallback callback) {
  auto* keyring = CreateDefaultKeyring(password);
  if (keyring) {
    AddAccountForDefaultKeyring(kFirstAccountName);
  }

  std::move(callback).Run(GetMnemonicForDefaultKeyringImpl());
}

void KeyringController::RestoreWallet(const std::string& mnemonic,
                                      const std::string& password,
                                      bool is_legacy_brave_wallet,
                                      RestoreWalletCallback callback) {
  auto* keyring =
      RestoreDefaultKeyring(mnemonic, password, is_legacy_brave_wallet);
  if (keyring && !keyring->GetAccountsNumber()) {
    AddAccountForDefaultKeyring(kFirstAccountName);
  }
  // TODO(darkdh): add account discovery mechanism

  std::move(callback).Run(keyring);
}

const std::string KeyringController::GetMnemonicForDefaultKeyringImpl() {
  if (IsLocked()) {
    VLOG(1) << __func__ << ": Must Unlock controller first";
    return std::string();
  }
  DCHECK(encryptor_);
  std::vector<uint8_t> encrypted_mnemonic;

  if (!GetPrefInBytesForKeyring(kEncryptedMnemonic, &encrypted_mnemonic,
                                kDefaultKeyringId)) {
    return std::string();
  }
  std::vector<uint8_t> mnemonic;
  if (!encryptor_->Decrypt(encrypted_mnemonic,
                           GetOrCreateNonceForKeyring(kDefaultKeyringId),
                           &mnemonic)) {
    return std::string();
  }

  return std::string(mnemonic.begin(), mnemonic.end());
}

void KeyringController::AddAccount(const std::string& account_name,
                                   AddAccountCallback callback) {
  auto* keyring = default_keyring_.get();
  if (keyring) {
    AddAccountForDefaultKeyring(account_name);
  }

  NotifyAccountsChanged();
  std::move(callback).Run(keyring);
}

void KeyringController::GetPrivateKeyForDefaultKeyringAccount(
    const std::string& address,
    GetPrivateKeyForDefaultKeyringAccountCallback callback) {
  if (address.empty() || !default_keyring_) {
    std::move(callback).Run(false, "");
    return;
  }

  auto* hd_key = default_keyring_->GetHDKeyFromAddress(address);
  if (!hd_key) {
    std::move(callback).Run(false, "");
    return;
  }

  std::string private_key = hd_key->GetHexEncodedPrivateKey();
  std::move(callback).Run(!private_key.empty(), private_key);
}

void KeyringController::ImportAccount(const std::string& account_name,
                                      const std::string& private_key_hex,
                                      ImportAccountCallback callback) {
  if (account_name.empty() || private_key_hex.empty() || !encryptor_) {
    std::move(callback).Run(false, "");
    return;
  }

  std::vector<uint8_t> private_key;
  if (!base::HexStringToBytes(private_key_hex, &private_key)) {
    std::move(callback).Run(false, "");
    return;
  }

  auto address = ImportAccountForDefaultKeyring(account_name, private_key);
  if (!address) {
    std::move(callback).Run(false, "");
    return;
  }

  std::move(callback).Run(true, *address);
}

void KeyringController::ImportAccountFromJson(const std::string& account_name,
                                              const std::string& password,
                                              const std::string& json,
                                              ImportAccountCallback callback) {
  if (account_name.empty() || password.empty() || json.empty() || !encryptor_) {
    std::move(callback).Run(false, "");
    return;
  }
  std::unique_ptr<HDKey> hd_key = HDKey::GenerateFromV3UTC(password, json);
  if (!hd_key) {
    std::move(callback).Run(false, "");
    return;
  }

  auto address =
      ImportAccountForDefaultKeyring(account_name, hd_key->private_key());
  if (!address) {
    std::move(callback).Run(false, "");
    return;
  }

  std::move(callback).Run(true, *address);
}

void KeyringController::GetPrivateKeyForImportedAccount(
    const std::string& address,
    GetPrivateKeyForImportedAccountCallback callback) {
  if (address.empty() || !encryptor_) {
    std::move(callback).Run(false, "");
    return;
  }
  for (const auto& imported_account_info :
       GetImportedAccountsForKeyring(prefs_, kDefaultKeyringId)) {
    if (imported_account_info.account_address == address) {
      std::string private_key_decoded;
      if (!base::Base64Decode(imported_account_info.encrypted_private_key,
                              &private_key_decoded))
        continue;
      std::vector<uint8_t> private_key;
      if (!encryptor_->Decrypt(ToSpan(private_key_decoded),
                               GetOrCreateNonceForKeyring(kDefaultKeyringId),
                               &private_key)) {
        continue;
      }
      std::move(callback).Run(true,
                              base::ToLowerASCII(base::HexEncode(private_key)));
      return;
    }
  }
  std::move(callback).Run(false, "");
}

void KeyringController::RemoveImportedAccount(
    const std::string& address,
    RemoveImportedAccountCallback callback) {
  if (address.empty() || !default_keyring_) {
    std::move(callback).Run(false);
    return;
  }

  if (!default_keyring_->RemoveImportedAccount(address)) {
    std::move(callback).Run(false);
    return;
  }
  RemoveImportedAccountForKeyring(prefs_, address, kDefaultKeyringId);

  NotifyAccountsChanged();
  std::move(callback).Run(true);
}

void KeyringController::IsWalletBackedUp(IsWalletBackedUpCallback callback) {
  bool backup_complete = false;
  const base::Value* value =
      GetPrefForKeyring(prefs_, kBackupComplete, kDefaultKeyringId);
  if (value)
    backup_complete = value->GetBool();
  std::move(callback).Run(backup_complete);
}

void KeyringController::NotifyWalletBackupComplete() {
  SetPrefForKeyring(prefs_, kBackupComplete, base::Value(true),
                    kDefaultKeyringId);
  for (const auto& observer : observers_) {
    observer->BackedUp();
  }
}

void KeyringController::AddAccountForDefaultKeyring(
    const std::string& account_name) {
  if (!default_keyring_)
    return;
  default_keyring_->AddAccounts(1);
  size_t accounts_num = default_keyring_->GetAccountsNumber();
  CHECK(accounts_num);
  SetAccountMetaForKeyring(
      prefs_, GetAccountPathByIndex(accounts_num - 1), account_name,
      default_keyring_->GetAddress(accounts_num - 1), kDefaultKeyringId);
}

absl::optional<std::string> KeyringController::ImportAccountForDefaultKeyring(
    const std::string& account_name,
    const std::vector<uint8_t>& private_key) {
  if (!default_keyring_) {
    return absl::nullopt;
  }

  const std::string address = default_keyring_->ImportAccount(private_key);
  if (address.empty()) {
    return absl::nullopt;
  }
  std::vector<uint8_t> encrypted_private_key;
  if (!encryptor_->Encrypt(private_key,
                           GetOrCreateNonceForKeyring(kDefaultKeyringId),
                           &encrypted_private_key)) {
    return absl::nullopt;
  }
  ImportedAccountInfo info = {account_name, address,
                              base::Base64Encode(encrypted_private_key)};
  SetImportedAccountForKeyring(prefs_, info, kDefaultKeyringId);

  NotifyAccountsChanged();

  return address;
}

size_t KeyringController::GetAccountMetasNumberForKeyring(
    const std::string& id) {
  const base::Value* account_metas =
      GetPrefForKeyring(prefs_, kAccountMetas, id);
  if (!account_metas)
    return 0;

  return account_metas->DictSize();
}

// This member function should not assume that the wallet is unlocked!
std::vector<mojom::AccountInfoPtr> KeyringController::GetAccountInfosForKeyring(
    const std::string& id) {
  std::vector<mojom::AccountInfoPtr> result;

  size_t account_no = GetAccountMetasNumberForKeyring(kDefaultKeyringId);
  for (size_t i = 0; i < account_no; ++i) {
    mojom::AccountInfoPtr account_info = mojom::AccountInfo::New();
    account_info->address =
        GetAccountAddressForKeyring(prefs_, GetAccountPathByIndex(i), id);
    account_info->name =
        GetAccountNameForKeyring(prefs_, GetAccountPathByIndex(i), id);
    account_info->is_imported = false;
    result.push_back(std::move(account_info));
  }
  // append imported account info
  for (const auto& imported_account_info :
       GetImportedAccountsForKeyring(prefs_, id)) {
    mojom::AccountInfoPtr account_info = mojom::AccountInfo::New();
    account_info->address = imported_account_info.account_address;
    account_info->name = imported_account_info.account_name;
    account_info->is_imported = true;
    result.push_back(std::move(account_info));
  }
  return result;
}

void KeyringController::GetHardwareAccounts(
    GetHardwareAccountsCallback callback) {
  std::vector<mojom::AccountInfoPtr> accounts;
  base::Value hardware_keyrings(base::Value::Type::DICTIONARY);
  const base::Value* value = GetPrefForHardwareKeyringUpdate(prefs_);
  if (!value) {
    std::move(callback).Run({});
    return;
  }

  for (const auto hw_keyring : value->DictItems()) {
    std::string device_id = hw_keyring.first;
    const base::Value* account_value = hw_keyring.second.FindKey(kAccountMetas);
    if (!account_value)
      continue;
    SerializeHardwareAccounts(account_value, &accounts);
  }

  std::move(callback).Run(std::move(accounts));
}

// static
base::Value* KeyringController::GetPrefForHardwareKeyringUpdate(
    PrefService* prefs) {
  DCHECK(prefs);
  DictionaryPrefUpdate update(prefs, kBraveWalletKeyrings);
  base::DictionaryValue* keyrings_pref = update.Get();
  if (!keyrings_pref)
    return nullptr;
  base::Value* keyring_dict = keyrings_pref->FindKey(kHardwareKeyrings);
  if (!keyring_dict)
    keyring_dict = keyrings_pref->SetKey(
        kHardwareKeyrings, base::Value(base::Value::Type::DICTIONARY));
  return keyring_dict;
}

void KeyringController::AddHardwareAccounts(
    std::vector<mojom::HardwareWalletAccountPtr> infos) {
  if (infos.empty())
    return;
  const auto& hardware_vendor = infos.front()->hardware_vendor;
  const auto hash = base::PersistentHash(infos.front()->address);
  std::string device_id = hardware_vendor + std::to_string(hash);

  base::Value* hardware_keyrings = GetPrefForHardwareKeyringUpdate(prefs_);

  base::Value* device_value = hardware_keyrings->FindKey(device_id);
  if (!device_value) {
    device_value = hardware_keyrings->SetKey(
        device_id, base::Value(base::Value::Type::DICTIONARY));
  }

  base::Value* meta_value = device_value->FindKey(kAccountMetas);
  if (!meta_value) {
    meta_value = device_value->SetKey(
        kAccountMetas, base::Value(base::Value::Type::DICTIONARY));
  }
  for (const auto& info : infos) {
    DCHECK_EQ(hardware_vendor, info->hardware_vendor);
    if (hardware_vendor != info->hardware_vendor)
      continue;
    base::Value hw_account(base::Value::Type::DICTIONARY);
    hw_account.SetStringKey(kAccountName, info->name);
    hw_account.SetStringKey(kHardwareVendor, info->hardware_vendor);
    hw_account.SetStringKey(kHardwareDerivationPath, info->derivation_path);

    meta_value->SetKey(info->address, std::move(hw_account));
  }

  NotifyAccountsChanged();
}

void KeyringController::RemoveHardwareAccount(const std::string& address) {
  base::Value* hardware_keyrings = GetPrefForHardwareKeyringUpdate(prefs_);
  for (auto devices : hardware_keyrings->DictItems()) {
    base::Value* account_metas = devices.second.FindKey(kAccountMetas);
    if (!account_metas)
      continue;
    const base::Value* address_key = account_metas->FindKey(address);
    if (!address_key)
      continue;
    account_metas->RemoveKey(address);

    if (account_metas->DictEmpty())
      hardware_keyrings->RemoveKey(devices.first);

    NotifyAccountsChanged();
    return;
  }
}

void KeyringController::SignTransactionByDefaultKeyring(
    const std::string& address,
    EthTransaction* tx,
    uint256_t chain_id) {
  if (!default_keyring_)
    return;
  default_keyring_->SignTransaction(address, tx, chain_id);
}

bool KeyringController::IsLocked() const {
  return encryptor_ == nullptr;
}

void KeyringController::Lock() {
  if (IsLocked() || !default_keyring_)
    return;
  default_keyring_.reset();

  encryptor_.reset();
  for (const auto& observer : observers_) {
    observer->Locked();
  }
}

void KeyringController::Unlock(const std::string& password,
                               UnlockCallback callback) {
  if (!ResumeDefaultKeyring(password)) {
    encryptor_.reset();
    std::move(callback).Run(false);
    return;
  }

  UpdateLastUnlockPref(prefs_);
  for (const auto& observer : observers_) {
    observer->Unlocked();
  }
  std::move(callback).Run(true);
}

void KeyringController::IsLocked(IsLockedCallback callback) {
  std::move(callback).Run(IsLocked());
}

void KeyringController::Reset() {
  encryptor_.reset();
  default_keyring_.reset();

  ClearProfilePrefs(prefs_);
}

bool KeyringController::GetPrefInBytesForKeyring(const std::string& key,
                                                 std::vector<uint8_t>* bytes,
                                                 const std::string& id) const {
  if (!bytes)
    return false;

  const base::Value* value = GetPrefForKeyring(prefs_, key, id);
  if (!value)
    return false;

  const std::string* encoded = value->GetIfString();
  if (!encoded || encoded->empty())
    return false;

  std::string decoded;
  if (!base::Base64Decode(*encoded, &decoded)) {
    return false;
  }
  *bytes = std::vector<uint8_t>(decoded.begin(), decoded.end());
  return true;
}

void KeyringController::SetPrefInBytesForKeyring(
    const std::string& key,
    base::span<const uint8_t> bytes,
    const std::string& id) {
  const std::string encoded = base::Base64Encode(bytes);
  SetPrefForKeyring(prefs_, key, base::Value(encoded), id);
}

std::vector<uint8_t> KeyringController::GetOrCreateNonceForKeyring(
    const std::string& id) {
  std::vector<uint8_t> nonce(kNonceSize);
  if (!GetPrefInBytesForKeyring(kPasswordEncryptorNonce, &nonce, id)) {
    crypto::RandBytes(nonce);
    SetPrefInBytesForKeyring(kPasswordEncryptorNonce, nonce, id);
  }
  return nonce;
}

bool KeyringController::CreateEncryptorForKeyring(const std::string& password,
                                                  const std::string& id) {
  if (password.empty())
    return false;
  std::vector<uint8_t> salt(kSaltSize);
  if (!GetPrefInBytesForKeyring(kPasswordEncryptorSalt, &salt, id)) {
    crypto::RandBytes(salt);
    SetPrefInBytesForKeyring(kPasswordEncryptorSalt, salt, id);
  }
  encryptor_ = PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
      password, salt, 100000, 256);
  return encryptor_ != nullptr;
}

bool KeyringController::CreateDefaultKeyringInternal(
    const std::string& mnemonic,
    bool is_legacy_brave_wallet) {
  if (!encryptor_)
    return false;

  std::unique_ptr<std::vector<uint8_t>> seed = nullptr;
  if (is_legacy_brave_wallet)
    seed = MnemonicToEntropy(mnemonic);
  else
    seed = MnemonicToSeed(mnemonic, "");
  if (!seed)
    return false;
  if (is_legacy_brave_wallet && seed->size() != 32) {
    VLOG(1) << __func__
            << "mnemonic for legacy brave wallet must be 24 words which will "
               "produce 32 bytes seed";
    return false;
  }

  std::vector<uint8_t> encrypted_mnemonic;
  if (!encryptor_->Encrypt(ToSpan(mnemonic),
                           GetOrCreateNonceForKeyring(kDefaultKeyringId),
                           &encrypted_mnemonic)) {
    return false;
  }

  SetPrefInBytesForKeyring(kEncryptedMnemonic, encrypted_mnemonic,
                           kDefaultKeyringId);
  if (is_legacy_brave_wallet)
    SetPrefForKeyring(prefs_, kLegacyBraveWallet, base::Value(true),
                      kDefaultKeyringId);
  else
    SetPrefForKeyring(prefs_, kLegacyBraveWallet, base::Value(false),
                      kDefaultKeyringId);

  default_keyring_ = std::make_unique<HDKeyring>();
  default_keyring_->ConstructRootHDKey(*seed, kRootPath);
  UpdateLastUnlockPref(prefs_);

  return true;
}

bool KeyringController::IsDefaultKeyringCreated() {
  return HasPrefForKeyring(prefs_, kEncryptedMnemonic, kDefaultKeyringId);
}

void KeyringController::AddObserver(
    ::mojo::PendingRemote<mojom::KeyringControllerObserver> observer) {
  observers_.Add(std::move(observer));
}

void KeyringController::SetDefaultKeyringDerivedAccountName(
    const std::string& address,
    const std::string& name,
    SetDefaultKeyringDerivedAccountNameCallback callback) {
  if (address.empty() || name.empty() || !default_keyring_) {
    std::move(callback).Run(false);
    return;
  }

  const absl::optional<size_t> index =
      default_keyring_->GetAccountIndex(address);
  if (!index) {
    std::move(callback).Run(false);
    return;
  }

  SetAccountMetaForKeyring(prefs_, GetAccountPathByIndex(index.value()), name,
                           address, kDefaultKeyringId);
  NotifyAccountsChanged();
  std::move(callback).Run(true);
}

void KeyringController::SetDefaultKeyringImportedAccountName(
    const std::string& address,
    const std::string& name,
    SetDefaultKeyringImportedAccountNameCallback callback) {
  if (address.empty() || name.empty()) {
    std::move(callback).Run(false);
    return;
  }

  base::Value imported_accounts(base::Value::Type::LIST);
  const base::Value* value =
      GetPrefForKeyring(prefs_, kImportedAccounts, kDefaultKeyringId);
  if (!value) {
    std::move(callback).Run(false);
    return;
  }

  imported_accounts = value->Clone();
  base::Value::ListView imported_accounts_list = imported_accounts.GetList();

  bool name_updated = false;
  for (size_t i = 0; i < imported_accounts_list.size(); ++i) {
    const std::string* account_address =
        imported_accounts_list[i].FindStringKey(kAccountAddress);
    if (account_address && *account_address == address) {
      imported_accounts_list[i].SetStringKey(kAccountName, name);
      SetPrefForKeyring(prefs_, kImportedAccounts, std::move(imported_accounts),
                        kDefaultKeyringId);
      NotifyAccountsChanged();
      name_updated = true;
      break;
    }
  }

  std::move(callback).Run(name_updated);
}

void KeyringController::NotifyAccountsChanged() {
  for (const auto& observer : observers_) {
    observer->AccountsChanged();
  }
}

}  // namespace brave_wallet
