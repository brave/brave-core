/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/keyring_service.h"

#include <utility>

#include "base/base64.h"
#include "base/hash/hash.h"
#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/value_iterators.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/filecoin_keyring.h"
#include "brave/components/brave_wallet/browser/hd_key.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/json_rpc_service.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/eth_address.h"
#include "brave/components/brave_wallet/common/hex_utils.h"
#include "components/grit/brave_components_strings.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "crypto/random.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/re2/src/re2/re2.h"
#include "ui/base/l10n/l10n_util.h"

/* kBraveWalletKeyrings structure
 *
 * {
 *   "hardware":  {
 *     "Ledger12445": {
 *        "account_metas": {
 *            "0xEA04...CC8Acc": {
 *              "account_name": "Ledger",
 *              "derivation_path": "m/44'/60'/1'/0/0",
 *              "hardware_vendor": "ledger",
 *              "coin": 60 // Enum mojom::CoinType
 *           },
 *           "0x264Ef...6b8F1": {
 *              "account_name": "Ledger",
 *              "derivation_path": "m/44'/60'/2'/0/0",
 *              "hardware_vendor": "ledger",
 *              "coin": 60 // Enum mojom::CoinType
 *            }
 *        },
 *        device_name: "Ledger 123"
 *     },
 *     "Ledger44332":{
 *      ...
 *     }
 *   }
 * },
 * "filecoin":
 *   {
 *     "imported_accounts": [
 *       {
 *           "account_address": "t3vmv....ughsa",
 *           "account_name": "Filecoin",
 *           "encrypted_private_key": "9/Xb...X4IL",
 *           "coin": 461 // Enum mojom::CoinType
 *       }
 *     ],
 *   "password_encryptor_nonce": "xxx"
 * },
 * "default":
 *   {  "backup_complete": false,
 *      "encrypted_mnemonic": [mnemonic],
 *      "legacy_brave_wallet": false,
 *      "account_metas": {
 *         "m/44'/60'/0'/0/0": {
 *               "account_name": "account 1",
 *               ...
 *               "coin": 60 // Enum mojom::CoinType
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
const char kCoinType[] = "coin_type";
const char kLegacyBraveWallet[] = "legacy_brave_wallet";
const char kHardwareKeyrings[] = "hardware";
const char kHardwareDerivationPath[] = "derivation_path";

std::string GetKeyringId(HDKeyring::Type type) {
  if (type == HDKeyring::kFilecoin)
    return mojom::kFilecoinKeyringId;
  return mojom::kDefaultKeyringId;
}

mojom::CoinType GetCoinForKeyring(const std::string& keyring_id) {
  if (keyring_id == mojom::kFilecoinKeyringId) {
    return mojom::CoinType::FIL;
  }
  DCHECK_EQ(keyring_id, mojom::kDefaultKeyringId);
  return mojom::CoinType::ETH;
}

static base::span<const uint8_t> ToSpan(base::StringPiece sp) {
  return base::as_bytes(base::make_span(sp));
}

std::string GetAccountName(size_t number) {
  return l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_NUMBERED_ACCOUNT_NAME,
                                   base::NumberToString16(number));
}

void SerializeHardwareAccounts(const std::string& device_id,
                               const base::Value* account_value,
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

    mojom::CoinType coin = mojom::CoinType::ETH;
    auto coin_name_value = account.second.FindIntKey(kCoinType);
    if (coin_name_value) {
      coin = static_cast<mojom::CoinType>(*coin_name_value);
    }

    accounts->push_back(mojom::AccountInfo::New(
        address, name, false,
        mojom::HardwareInfo::New(derivation_path, hardware_vendor, device_id),
        coin));
  }
}

}  // namespace

KeyringService::KeyringService(PrefService* prefs) : prefs_(prefs) {
  DCHECK(prefs);
  auto_lock_timer_ = std::make_unique<base::OneShotTimer>();

  pref_change_registrar_ = std::make_unique<PrefChangeRegistrar>();
  pref_change_registrar_->Init(prefs);
  pref_change_registrar_->Add(
      kBraveWalletAutoLockMinutes,
      base::BindRepeating(&KeyringService::OnAutoLockPreferenceChanged,
                          base::Unretained(this)));
  pref_change_registrar_->Add(
      kBraveWalletSelectedAccount,
      base::BindRepeating(&KeyringService::OnSelectedAccountPreferenceChanged,
                          base::Unretained(this)));
}

KeyringService::~KeyringService() {
  auto_lock_timer_.reset();
}

mojo::PendingRemote<mojom::KeyringService> KeyringService::MakeRemote() {
  mojo::PendingRemote<mojom::KeyringService> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

KeyringService::ImportedAccountInfo::ImportedAccountInfo(
    const std::string& input_account_name,
    const std::string& input_account_address,
    const std::string& input_encrypted_private_key,
    mojom::CoinType input_coin)
    : account_name(input_account_name),
      account_address(input_account_address),
      encrypted_private_key(input_encrypted_private_key),
      coin(input_coin) {}

KeyringService::ImportedAccountInfo::~ImportedAccountInfo() {}

KeyringService::ImportedAccountInfo::ImportedAccountInfo(
    const KeyringService::ImportedAccountInfo& other) {
  account_name = other.account_name;
  account_address = other.account_address;
  encrypted_private_key = other.encrypted_private_key;
  coin = other.coin;
}

void KeyringService::Bind(
    mojo::PendingReceiver<mojom::KeyringService> receiver) {
  receivers_.Add(this, std::move(receiver));
}

// static
void KeyringService::MigrateObsoleteProfilePrefs(PrefService* prefs) {
  if (prefs->HasPrefPath(kBraveWalletPasswordEncryptorSalt) &&
      prefs->HasPrefPath(kBraveWalletPasswordEncryptorNonce) &&
      prefs->HasPrefPath(kBraveWalletEncryptedMnemonic)) {
    SetPrefForKeyring(
        prefs, kPasswordEncryptorSalt,
        base::Value(prefs->GetString(kBraveWalletPasswordEncryptorSalt)),
        mojom::kDefaultKeyringId);
    SetPrefForKeyring(
        prefs, kPasswordEncryptorNonce,
        base::Value(prefs->GetString(kBraveWalletPasswordEncryptorNonce)),
        mojom::kDefaultKeyringId);
    SetPrefForKeyring(
        prefs, kEncryptedMnemonic,
        base::Value(prefs->GetString(kBraveWalletEncryptedMnemonic)),
        mojom::kDefaultKeyringId);
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
                                 mojom::kDefaultKeyringId);
      }
    } else {
      // This shouldn't happen but we will reset account to default state as
      // fail-safe
      SetAccountMetaForKeyring(prefs, GetAccountPathByIndex(0),
                               GetAccountName(1), "", mojom::kDefaultKeyringId);
    }
    prefs->ClearPref(kBraveWalletDefaultKeyringAccountNum);
    prefs->ClearPref(kBraveWalletAccountNames);
  }

  if (prefs->HasPrefPath(kBraveWalletBackupComplete)) {
    SetPrefForKeyring(
        prefs, kBackupComplete,
        base::Value(prefs->GetBoolean(kBraveWalletBackupComplete)),
        mojom::kDefaultKeyringId);
    prefs->ClearPref(kBraveWalletBackupComplete);
  }
}

// static
bool KeyringService::HasPrefForKeyring(PrefService* prefs,
                                       const std::string& key,
                                       const std::string& id) {
  return GetPrefForKeyring(prefs, key, id) != nullptr;
}

// static
const base::Value* KeyringService::GetPrefForKeyring(PrefService* prefs,
                                                     const std::string& key,
                                                     const std::string& id) {
  DCHECK(prefs);
  const base::Value* keyrings_pref = prefs->GetDictionary(kBraveWalletKeyrings);
  if (!keyrings_pref)
    return nullptr;
  const base::Value* keyring_dict = keyrings_pref->FindKey(id);
  if (!keyring_dict)
    return nullptr;

  return keyring_dict->FindKey(key);
}

// static
base::Value* KeyringService::GetPrefForKeyringUpdate(PrefService* prefs,
                                                     const std::string& key,
                                                     const std::string& id) {
  DCHECK(prefs);
  DictionaryPrefUpdate update(prefs, kBraveWalletKeyrings);
  base::Value* keyrings_pref = update.Get();
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
void KeyringService::SetPrefForKeyring(PrefService* prefs,
                                       const std::string& key,
                                       base::Value value,
                                       const std::string& id) {
  DCHECK(prefs);
  DictionaryPrefUpdate update(prefs, kBraveWalletKeyrings);
  base::Value* keyrings_pref = update.Get();

  if (!keyrings_pref->FindKey(id)) {
    keyrings_pref->SetKey(id, base::Value(base::Value::Type::DICTIONARY));
  }

  base::Value* keyring_dict = keyrings_pref->FindKey(id);
  if (!keyring_dict)
    return;

  keyring_dict->SetKey(key, std::move(value));
}

// static
void KeyringService::SetAccountMetaForKeyring(
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
std::string KeyringService::GetAccountNameForKeyring(
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
std::string KeyringService::GetAccountAddressForKeyring(
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
std::string KeyringService::GetAccountPathByIndex(size_t index) {
  return std::string(kRootPath) + "/" + base::NumberToString(index);
}

// static
void KeyringService::SetImportedAccountForKeyring(
    PrefService* prefs,
    const ImportedAccountInfo& info,
    const std::string& id) {
  base::Value imported_account(base::Value::Type::DICTIONARY);
  imported_account.SetStringKey(kAccountName, info.account_name);
  imported_account.SetStringKey(kAccountAddress, info.account_address);
  imported_account.SetStringKey(kEncryptedPrivateKey,
                                info.encrypted_private_key);
  imported_account.SetIntKey(kCoinType, static_cast<int>(info.coin));

  base::Value imported_accounts(base::Value::Type::LIST);
  const base::Value* value = GetPrefForKeyring(prefs, kImportedAccounts, id);
  if (value)
    imported_accounts = value->Clone();
  imported_accounts.Append(std::move(imported_account));

  SetPrefForKeyring(prefs, kImportedAccounts, std::move(imported_accounts), id);
}

// static
std::vector<KeyringService::ImportedAccountInfo>
KeyringService::GetImportedAccountsForKeyring(PrefService* prefs,
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
    mojom::CoinType coin = GetCoinForKeyring(id);
    auto coin_name_value = imported_account.FindIntKey(kCoinType);
    if (coin_name_value) {
      coin = static_cast<mojom::CoinType>(*coin_name_value);
    }

    result.push_back(ImportedAccountInfo(*account_name, *account_address,
                                         *encrypted_private_key, coin));
  }
  return result;
}
// static
void KeyringService::RemoveImportedAccountForKeyring(PrefService* prefs,
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

HDKeyring* KeyringService::CreateKeyring(const std::string& keyring_id,
                                         const std::string& password) {
  if (keyring_id != mojom::kDefaultKeyringId &&
      keyring_id != mojom::kFilecoinKeyringId) {
    VLOG(1) << "Unknown keyring id " << keyring_id;
    return nullptr;
  }
  if (!CreateEncryptorForKeyring(password, keyring_id))
    return nullptr;

  const std::string mnemonic = GenerateMnemonic(16);
  if (!CreateKeyringInternal(keyring_id, mnemonic, false)) {
    return nullptr;
  }

  for (const auto& observer : observers_) {
    observer->KeyringCreated();
  }
  ResetAutoLockTimer();

  return GetHDKeyringById(keyring_id);
}

void KeyringService::RequestUnlock() {
  DCHECK(IsLocked());
  request_unlock_pending_ = true;
}

HDKeyring* KeyringService::ResumeKeyring(const std::string& keyring_id,
                                         const std::string& password) {
  if (!CreateEncryptorForKeyring(password, keyring_id)) {
    return nullptr;
  }

  const std::string mnemonic = GetMnemonicForKeyringImpl(keyring_id);
  bool is_legacy_brave_wallet = false;
  const base::Value* value =
      GetPrefForKeyring(prefs_, kLegacyBraveWallet, keyring_id);
  if (value)
    is_legacy_brave_wallet = value->GetBool();
  if (mnemonic.empty() ||
      !CreateKeyringInternal(keyring_id, mnemonic, is_legacy_brave_wallet)) {
    return nullptr;
  }

  auto* keyring = GetHDKeyringById(keyring_id);
  size_t account_no = GetAccountMetasNumberForKeyring(keyring_id);
  if (account_no)
    keyring->AddAccounts(account_no);

  // TODO(bbondy):
  // We can remove this some months after the initial wallet launch
  // We didn't store account address in meta pref originally.
  for (size_t i = 0; i < account_no; ++i) {
    SetAccountMetaForKeyring(prefs_, GetAccountPathByIndex(i), absl::nullopt,
                             keyring->GetAddress(i), keyring_id);
  }

  for (const auto& imported_account_info :
       GetImportedAccountsForKeyring(prefs_, keyring_id)) {
    std::string private_key_decoded;
    if (!base::Base64Decode(imported_account_info.encrypted_private_key,
                            &private_key_decoded))
      continue;
    std::vector<uint8_t> private_key;
    if (!encryptor_->Decrypt(ToSpan(private_key_decoded),
                             GetOrCreateNonceForKeyring(keyring_id),
                             &private_key)) {
      continue;
    }
    if (keyring_id == mojom::kFilecoinKeyringId) {
      auto* filecoin_keyring = static_cast<FilecoinKeyring*>(keyring);
      if (filecoin_keyring) {
        filecoin_keyring->ImportFilecoinAccount(
            private_key, imported_account_info.account_address);
      }
    } else if (keyring_id == mojom::kDefaultKeyringId) {
      keyring->ImportAccount(private_key);
    }
  }

  return keyring;
}

HDKeyring* KeyringService::RestoreKeyring(const std::string& keyring_id,
                                          const std::string& mnemonic,
                                          const std::string& password,
                                          bool is_legacy_brave_wallet) {
  if (!IsValidMnemonic(mnemonic))
    return nullptr;

  // Try getting existing mnemonic first
  if (CreateEncryptorForKeyring(password, keyring_id)) {
    const std::string current_mnemonic = GetMnemonicForKeyringImpl(keyring_id);
    // Restore with same mnmonic and same password, resume current keyring
    // Also need to make sure is_legacy_brave_wallet are the same, users might
    // choose the option wrongly and then want to start over with same mnemonic
    // but different is_legacy_brave_wallet value
    const base::Value* value =
        GetPrefForKeyring(prefs_, kLegacyBraveWallet, keyring_id);
    if (!current_mnemonic.empty() && current_mnemonic == mnemonic && value &&
        value->GetBool() == is_legacy_brave_wallet) {
      return ResumeKeyring(keyring_id, password);
    } else {
      // We have no way to check if new mnemonic is same as current mnemonic so
      // we need to clear all prefs for fresh start
      Reset(false);
    }
  }

  if (!CreateEncryptorForKeyring(password, keyring_id)) {
    return nullptr;
  }

  if (!CreateKeyringInternal(keyring_id, mnemonic, is_legacy_brave_wallet)) {
    return nullptr;
  }

  for (const auto& observer : observers_) {
    observer->KeyringRestored();
  }
  ResetAutoLockTimer();
  return GetHDKeyringById(keyring_id);
}

mojom::KeyringInfoPtr KeyringService::GetKeyringInfoSync(
    const std::string& keyring_id) {
  mojom::KeyringInfoPtr keyring_info = mojom::KeyringInfo::New();
  keyring_info->id = keyring_id;
  keyring_info->is_default_keyring_created = IsKeyringCreated(keyring_id);
  keyring_info->is_locked = IsLocked();
  bool backup_complete = false;
  const base::Value* value =
      GetPrefForKeyring(prefs_, kBackupComplete, keyring_id);
  if (value)
    backup_complete = value->GetBool();
  keyring_info->is_backed_up = backup_complete;
  keyring_info->account_infos = GetAccountInfosForKeyring(keyring_id);
  return keyring_info;
}

void KeyringService::GetKeyringInfo(const std::string& keyring_id,
                                    GetKeyringInfoCallback callback) {
  std::move(callback).Run(GetKeyringInfoSync(keyring_id));
}

void KeyringService::GetKeyringsInfo(const std::vector<std::string>& keyrings,
                                     GetKeyringsInfoCallback callback) {
  std::vector<mojom::KeyringInfoPtr> result;
  result.push_back(GetKeyringInfoSync(mojom::kDefaultKeyringId));
  if (IsFilecoinEnabled()) {
    result.push_back(GetKeyringInfoSync(mojom::kFilecoinKeyringId));
  }
  std::move(callback).Run(std::move(result));
}

void KeyringService::GetMnemonicForDefaultKeyring(
    GetMnemonicForDefaultKeyringCallback callback) {
  std::move(callback).Run(GetMnemonicForKeyringImpl(mojom::kDefaultKeyringId));
}

void KeyringService::CreateWallet(const std::string& password,
                                  CreateWalletCallback callback) {
  auto* keyring = CreateKeyring(mojom::kDefaultKeyringId, password);
  if (keyring) {
    AddAccountForDefaultKeyring(GetAccountName(1));
  }
  std::move(callback).Run(GetMnemonicForKeyringImpl(mojom::kDefaultKeyringId));
}

void KeyringService::RestoreWallet(const std::string& mnemonic,
                                   const std::string& password,
                                   bool is_legacy_brave_wallet,
                                   RestoreWalletCallback callback) {
  auto* keyring = RestoreKeyring(mojom::kDefaultKeyringId, mnemonic, password,
                                 is_legacy_brave_wallet);
  if (keyring && !keyring->GetAccountsNumber()) {
    AddAccountForDefaultKeyring(GetAccountName(1));
  }
  // TODO(darkdh): add account discovery mechanism

  std::move(callback).Run(keyring);
}

const std::string KeyringService::GetMnemonicForKeyringImpl(
    const std::string& keyring_id) {
  if (IsLocked()) {
    VLOG(1) << __func__ << ": Must Unlock service first";
    return std::string();
  }
  DCHECK(encryptor_);
  std::vector<uint8_t> encrypted_mnemonic;

  if (!GetPrefInBytesForKeyring(kEncryptedMnemonic, &encrypted_mnemonic,
                                keyring_id)) {
    return std::string();
  }
  std::vector<uint8_t> mnemonic;
  if (!encryptor_->Decrypt(encrypted_mnemonic,
                           GetOrCreateNonceForKeyring(keyring_id), &mnemonic)) {
    return std::string();
  }

  return std::string(mnemonic.begin(), mnemonic.end());
}

void KeyringService::AddAccount(const std::string& account_name,
                                AddAccountCallback callback) {
  auto* keyring = GetHDKeyringById(mojom::kDefaultKeyringId);
  if (keyring) {
    AddAccountForDefaultKeyring(account_name);
  }

  NotifyAccountsChanged();
  std::move(callback).Run(keyring);
}

void KeyringService::GetPrivateKeyForDefaultKeyringAccount(
    const std::string& address,
    GetPrivateKeyForDefaultKeyringAccountCallback callback) {
  auto* keyring = GetHDKeyringById(mojom::kDefaultKeyringId);
  if (address.empty() || !keyring) {
    std::move(callback).Run(false, "");
    return;
  }

  auto* hd_key = keyring->GetHDKeyFromAddress(address);
  if (!hd_key) {
    std::move(callback).Run(false, "");
    return;
  }

  std::string private_key = hd_key->GetHexEncodedPrivateKey();
  std::move(callback).Run(!private_key.empty(), private_key);
}

bool KeyringService::IsFilecoinAccount(const std::string& account) const {
  for (const auto& filecoin_account_info :
       GetImportedAccountsForKeyring(prefs_, mojom::kFilecoinKeyringId)) {
    mojom::AccountInfoPtr account_info = mojom::AccountInfo::New();
    if (account == filecoin_account_info.account_address) {
      return true;
    }
  }
  return false;
}

void KeyringService::ImportFilecoinSECP256K1Account(
    const std::string& account_name,
    const std::string& private_key_hex,
    const std::string& network,
    ImportFilecoinSECP256K1AccountCallback callback) {
  if (!IsFilecoinEnabled()) {
    VLOG(1) << "Filecoin feature is not enabled";
    return;
  }
  if (account_name.empty() || private_key_hex.empty() || !encryptor_) {
    std::move(callback).Run(false, "");
    return;
  }
  std::vector<uint8_t> private_key;
  if (!base::HexStringToBytes(private_key_hex, &private_key)) {
    std::move(callback).Run(false, "");
    return;
  }
  auto address = ImportSECP256K1AccountForFilecoinKeyring(account_name,
                                                          private_key, network);
  if (!address) {
    std::move(callback).Run(false, "");
    return;
  }

  std::move(callback).Run(true, *address);
}

void KeyringService::ImportFilecoinBLSAccount(
    const std::string& account_name,
    const std::string& private_key_hex,
    const std::string& public_key_hex,
    const std::string& network,
    ImportFilecoinBLSAccountCallback callback) {
  if (!IsFilecoinEnabled()) {
    VLOG(1) << "Filecoin feature is not enabled";
    std::move(callback).Run(false, "");
    return;
  }

  if (account_name.empty() || private_key_hex.empty() ||
      public_key_hex.empty() || !encryptor_) {
    std::move(callback).Run(false, "");
    return;
  }
  std::vector<uint8_t> private_key;
  if (!base::HexStringToBytes(private_key_hex, &private_key)) {
    std::move(callback).Run(false, "");
    return;
  }
  std::vector<uint8_t> public_key;
  if (!base::HexStringToBytes(public_key_hex, &public_key)) {
    std::move(callback).Run(false, "");
    return;
  }

  auto address = ImportBLSAccountForFilecoinKeyring(account_name, private_key,
                                                    public_key, network);
  if (!address) {
    std::move(callback).Run(false, "");
    return;
  }

  std::move(callback).Run(true, *address);
}

absl::optional<std::string>
KeyringService::ImportSECP256K1AccountForFilecoinKeyring(
    const std::string& account_name,
    const std::vector<uint8_t>& private_key,
    const std::string& network) {
  auto* keyring = static_cast<FilecoinKeyring*>(
      GetHDKeyringById(mojom::kFilecoinKeyringId));
  if (!keyring || !IsFilecoinEnabled()) {
    return absl::nullopt;
  }

  const std::string address =
      keyring->ImportFilecoinSECP256K1Account(private_key, network);
  if (address.empty()) {
    return absl::nullopt;
  }
  std::vector<uint8_t> encrypted_key;
  if (!encryptor_->Encrypt(
          private_key, GetOrCreateNonceForKeyring(mojom::kFilecoinKeyringId),
          &encrypted_key)) {
    return absl::nullopt;
  }
  ImportedAccountInfo info(account_name, address,
                           base::Base64Encode(encrypted_key),
                           mojom::CoinType::FIL);
  SetImportedAccountForKeyring(prefs_, info, mojom::kFilecoinKeyringId);

  NotifyAccountsChanged();

  return address;
}

absl::optional<std::string> KeyringService::ImportBLSAccountForFilecoinKeyring(
    const std::string& account_name,
    const std::vector<uint8_t>& private_key,
    const std::vector<uint8_t>& public_key,
    const std::string& network) {
  auto* keyring = static_cast<FilecoinKeyring*>(
      GetHDKeyringById(mojom::kFilecoinKeyringId));
  if (!keyring || !IsFilecoinEnabled()) {
    return absl::nullopt;
  }

  const std::string address =
      keyring->ImportFilecoinBLSAccount(private_key, public_key, network);
  if (address.empty()) {
    return absl::nullopt;
  }
  std::vector<uint8_t> encrypted_key;
  if (!encryptor_->Encrypt(
          private_key, GetOrCreateNonceForKeyring(mojom::kFilecoinKeyringId),
          &encrypted_key)) {
    return absl::nullopt;
  }
  ImportedAccountInfo info(account_name, address,
                           base::Base64Encode(encrypted_key),
                           mojom::CoinType::FIL);
  SetImportedAccountForKeyring(prefs_, info, mojom::kFilecoinKeyringId);

  NotifyAccountsChanged();

  return address;
}

void KeyringService::ImportAccount(const std::string& account_name,
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

void KeyringService::ImportAccountFromJson(const std::string& account_name,
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

std::vector<uint8_t> KeyringService::GetPrivateKeyFromKeyring(
    const std::string& address,
    const std::string& keyring_id) {
  for (const auto& imported_account_info :
       GetImportedAccountsForKeyring(prefs_, keyring_id)) {
    if (imported_account_info.account_address == address) {
      std::string private_key_decoded;
      if (!base::Base64Decode(imported_account_info.encrypted_private_key,
                              &private_key_decoded))
        continue;
      std::vector<uint8_t> private_key;
      if (!encryptor_->Decrypt(ToSpan(private_key_decoded),
                               GetOrCreateNonceForKeyring(keyring_id),
                               &private_key)) {
        continue;
      }
      return private_key;
    }
  }
  return std::vector<uint8_t>();
}

void KeyringService::GetPrivateKeyForImportedAccount(
    const std::string& address,
    GetPrivateKeyForImportedAccountCallback callback) {
  if (address.empty() || !encryptor_) {
    std::move(callback).Run(false, "");
    return;
  }
  auto* keyring = GetKeyringForAddress(address);
  std::vector<uint8_t> private_key =
      GetPrivateKeyFromKeyring(address, GetKeyringId(keyring->type()));
  if (!private_key.empty()) {
    std::move(callback).Run(true,
                            base::ToLowerASCII(base::HexEncode(private_key)));
    return;
  }
  std::move(callback).Run(false, "");
}

HDKeyring* KeyringService::GetKeyringForAddress(const std::string& address) {
  std::string keyring_id = IsFilecoinAccount(address)
                               ? brave_wallet::mojom::kFilecoinKeyringId
                               : brave_wallet::mojom::kDefaultKeyringId;
  return GetHDKeyringById(keyring_id);
}

HDKeyring* KeyringService::GetHDKeyringById(
    const std::string& keyring_id) const {
  if (keyrings_.contains(keyring_id))
    return keyrings_.at(keyring_id).get();
  return nullptr;
}

void KeyringService::RemoveImportedAccount(
    const std::string& address,
    RemoveImportedAccountCallback callback) {
  auto* keyring = GetKeyringForAddress(address);
  if (address.empty() || !keyring) {
    std::move(callback).Run(false);
    return;
  }

  if (!keyring->RemoveImportedAccount(address)) {
    std::move(callback).Run(false);
    return;
  }
  RemoveImportedAccountForKeyring(prefs_, address,
                                  GetKeyringId(keyring->type()));

  NotifyAccountsChanged();
  if (address == prefs_->GetString(kBraveWalletSelectedAccount)) {
    prefs_->ClearPref(kBraveWalletSelectedAccount);
  }
  std::move(callback).Run(true);
}

void KeyringService::IsWalletBackedUp(IsWalletBackedUpCallback callback) {
  bool backup_complete = false;
  const base::Value* value =
      GetPrefForKeyring(prefs_, kBackupComplete, mojom::kDefaultKeyringId);
  if (value)
    backup_complete = value->GetBool();
  std::move(callback).Run(backup_complete);
}

void KeyringService::NotifyWalletBackupComplete() {
  SetPrefForKeyring(prefs_, kBackupComplete, base::Value(true),
                    mojom::kDefaultKeyringId);
  for (const auto& observer : observers_) {
    observer->BackedUp();
  }
}

void KeyringService::AddAccountForDefaultKeyring(
    const std::string& account_name) {
  auto* keyring = GetHDKeyringById(mojom::kDefaultKeyringId);
  if (!keyring)
    return;
  keyring->AddAccounts(1);
  size_t accounts_num = keyring->GetAccountsNumber();
  CHECK(accounts_num);
  SetAccountMetaForKeyring(prefs_, GetAccountPathByIndex(accounts_num - 1),
                           account_name, keyring->GetAddress(accounts_num - 1),
                           mojom::kDefaultKeyringId);
}

absl::optional<std::string> KeyringService::ImportAccountForDefaultKeyring(
    const std::string& account_name,
    const std::vector<uint8_t>& private_key) {
  auto* keyring = GetHDKeyringById(mojom::kDefaultKeyringId);
  if (!keyring) {
    return absl::nullopt;
  }

  const std::string address = keyring->ImportAccount(private_key);
  if (address.empty()) {
    return absl::nullopt;
  }
  std::vector<uint8_t> encrypted_private_key;
  if (!encryptor_->Encrypt(private_key,
                           GetOrCreateNonceForKeyring(mojom::kDefaultKeyringId),
                           &encrypted_private_key)) {
    return absl::nullopt;
  }
  ImportedAccountInfo info(account_name, address,
                           base::Base64Encode(encrypted_private_key),
                           mojom::CoinType::ETH);
  SetImportedAccountForKeyring(prefs_, info, mojom::kDefaultKeyringId);

  NotifyAccountsChanged();

  return address;
}

size_t KeyringService::GetAccountMetasNumberForKeyring(const std::string& id) {
  const base::Value* account_metas =
      GetPrefForKeyring(prefs_, kAccountMetas, id);
  if (!account_metas)
    return 0;

  return account_metas->DictSize();
}

// This member function should not assume that the wallet is unlocked!
std::vector<mojom::AccountInfoPtr> KeyringService::GetAccountInfosForKeyring(
    const std::string& keyring_id) {
  std::vector<mojom::AccountInfoPtr> result;

  size_t account_no = GetAccountMetasNumberForKeyring(keyring_id);
  for (size_t i = 0; i < account_no; ++i) {
    mojom::AccountInfoPtr account_info = mojom::AccountInfo::New();
    account_info->address = GetAccountAddressForKeyring(
        prefs_, GetAccountPathByIndex(i), keyring_id);
    account_info->name =
        GetAccountNameForKeyring(prefs_, GetAccountPathByIndex(i), keyring_id);
    account_info->is_imported = false;
    account_info->coin = GetCoinForKeyring(keyring_id);
    result.push_back(std::move(account_info));
  }
  // append imported account info
  for (const auto& imported_account_info :
       GetImportedAccountsForKeyring(prefs_, keyring_id)) {
    mojom::AccountInfoPtr account_info = mojom::AccountInfo::New();
    account_info->address = imported_account_info.account_address;
    account_info->name = imported_account_info.account_name;
    account_info->is_imported = true;
    account_info->coin = imported_account_info.coin;
    result.push_back(std::move(account_info));
  }

  // append hardware accounts info
  for (const auto& hardware_account_info : GetHardwareAccountsSync()) {
    result.push_back(hardware_account_info.Clone());
  }
  return result;
}

std::vector<mojom::AccountInfoPtr> KeyringService::GetHardwareAccountsSync()
    const {
  std::vector<mojom::AccountInfoPtr> accounts;
  base::Value hardware_keyrings(base::Value::Type::DICTIONARY);
  const base::Value* value = GetPrefForHardwareKeyringUpdate(prefs_);
  if (!value) {
    return {};
  }

  for (const auto hw_keyring : value->DictItems()) {
    std::string device_id = hw_keyring.first;
    const base::Value* account_value = hw_keyring.second.FindKey(kAccountMetas);
    if (!account_value)
      continue;
    SerializeHardwareAccounts(device_id, account_value, &accounts);
  }

  return accounts;
}

// static
base::Value* KeyringService::GetPrefForHardwareKeyringUpdate(
    PrefService* prefs) {
  DCHECK(prefs);
  DictionaryPrefUpdate update(prefs, kBraveWalletKeyrings);
  base::Value* keyrings_pref = update.Get();
  if (!keyrings_pref)
    return nullptr;
  base::Value* keyring_dict = keyrings_pref->FindKey(kHardwareKeyrings);
  if (!keyring_dict)
    keyring_dict = keyrings_pref->SetKey(
        kHardwareKeyrings, base::Value(base::Value::Type::DICTIONARY));
  return keyring_dict;
}

void KeyringService::AddHardwareAccounts(
    std::vector<mojom::HardwareWalletAccountPtr> infos) {
  if (infos.empty())
    return;

  base::Value* hardware_keyrings = GetPrefForHardwareKeyringUpdate(prefs_);

  for (const auto& info : infos) {
    const auto& hardware_vendor = info->hardware_vendor;
    std::string device_id = info->device_id;
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

void KeyringService::RemoveHardwareAccount(const std::string& address) {
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
    if (address == prefs_->GetString(kBraveWalletSelectedAccount)) {
      prefs_->ClearPref(kBraveWalletSelectedAccount);
    }
    return;
  }
}

void KeyringService::SignTransactionByDefaultKeyring(const std::string& address,
                                                     EthTransaction* tx,
                                                     uint256_t chain_id) {
  auto* keyring = GetHDKeyringById(mojom::kDefaultKeyringId);
  if (!keyring)
    return;
  keyring->SignTransaction(address, tx, chain_id);
}

KeyringService::SignatureWithError::SignatureWithError() = default;
KeyringService::SignatureWithError::SignatureWithError(
    SignatureWithError&& other) = default;
KeyringService::SignatureWithError&
KeyringService::SignatureWithError::operator=(SignatureWithError&& other) =
    default;
KeyringService::SignatureWithError::~SignatureWithError() = default;

KeyringService::SignatureWithError KeyringService::SignMessageByDefaultKeyring(
    const std::string& address,
    const std::vector<uint8_t>& message,
    bool is_eip712) {
  SignatureWithError ret;
  auto* keyring = GetHDKeyringById(mojom::kDefaultKeyringId);
  if (!keyring) {
    ret.signature = absl::nullopt;
    ret.error_message =
        l10n_util::GetStringUTF8(IDS_BRAVE_WALLET_SIGN_MESSAGE_UNLOCK_FIRST);
    return ret;
  }

  // MM currently doesn't provide chain_id when signing message
  std::vector<uint8_t> signature =
      keyring->SignMessage(address, message, 0, is_eip712);
  if (signature.empty()) {
    ret.signature = absl::nullopt;
    ret.error_message =
        l10n_util::GetStringFUTF8(IDS_BRAVE_WALLET_SIGN_MESSAGE_INVALID_ADDRESS,
                                  base::ASCIIToUTF16(address));
    return ret;
  }
  ret.signature = std::move(signature);
  return ret;
}

bool KeyringService::RecoverAddressByDefaultKeyring(
    const std::vector<uint8_t>& message,
    const std::vector<uint8_t>& signature,
    std::string* address) {
  CHECK(address);
  return HDKeyring::RecoverAddress(message, signature, address);
}

void KeyringService::AddAccountsWithDefaultName(size_t number) {
  auto* keyring = GetHDKeyringById(mojom::kDefaultKeyringId);
  if (!keyring) {
    DCHECK(false) << "Should only be called when default keyring exists";
    return;
  }

  size_t current_num = keyring->GetAccountsNumber();
  for (size_t i = current_num + 1; i <= current_num + number; ++i) {
    AddAccountForDefaultKeyring(GetAccountName(i));
  }
}

bool KeyringService::IsLocked() const {
  return encryptor_ == nullptr;
}

bool KeyringService::HasPendingUnlockRequest() const {
  return request_unlock_pending_;
}

absl::optional<std::string> KeyringService::GetSelectedAccount() const {
  std::string address = prefs_->GetString(kBraveWalletSelectedAccount);
  if (address.empty()) {
    return absl::nullopt;
  }
  return address;
}

void KeyringService::Lock() {
  if (IsLocked())
    return;

  keyrings_.clear();

  encryptor_.reset();
  for (const auto& observer : observers_) {
    observer->Locked();
  }
  StopAutoLockTimer();
}

bool KeyringService::IsHardwareAccount(const std::string& account) const {
  for (const auto& hardware_account_info : GetHardwareAccountsSync()) {
    if (hardware_account_info->address == account) {
      return true;
    }
  }
  return false;
}

void KeyringService::Unlock(const std::string& password,
                            KeyringService::UnlockCallback callback) {
  if (!ResumeKeyring(mojom::kDefaultKeyringId, password)) {
    encryptor_.reset();
    std::move(callback).Run(false);
    return;
  }
  UpdateLastUnlockPref(prefs_);
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
  std::move(callback).Run(IsLocked());
}

void KeyringService::Reset(bool notify_observer) {
  StopAutoLockTimer();
  encryptor_.reset();
  keyrings_.clear();

  ClearKeyringServiceProfilePrefs(prefs_);

  if (notify_observer) {
    for (const auto& observer : observers_) {
      observer->KeyringReset();
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
        (size_t)prefs_->GetInteger(kBraveWalletAutoLockMinutes);
    auto_lock_timer_->Start(FROM_HERE, base::Minutes(auto_lock_minutes), this,
                            &KeyringService::OnAutoLockFired);
  }
}

bool KeyringService::GetPrefInBytesForKeyring(const std::string& key,
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

void KeyringService::SetPrefInBytesForKeyring(const std::string& key,
                                              base::span<const uint8_t> bytes,
                                              const std::string& id) {
  const std::string encoded = base::Base64Encode(bytes);
  SetPrefForKeyring(prefs_, key, base::Value(encoded), id);
}

std::vector<uint8_t> KeyringService::GetOrCreateNonceForKeyring(
    const std::string& id) {
  std::vector<uint8_t> nonce(kNonceSize);
  if (!GetPrefInBytesForKeyring(kPasswordEncryptorNonce, &nonce, id)) {
    crypto::RandBytes(nonce);
    SetPrefInBytesForKeyring(kPasswordEncryptorNonce, nonce, id);
  }
  return nonce;
}

bool KeyringService::CreateEncryptorForKeyring(const std::string& password,
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

bool KeyringService::CreateKeyringInternal(const std::string& keyring_id,
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
                           GetOrCreateNonceForKeyring(keyring_id),
                           &encrypted_mnemonic)) {
    return false;
  }

  SetPrefInBytesForKeyring(kEncryptedMnemonic, encrypted_mnemonic, keyring_id);
  if (is_legacy_brave_wallet)
    SetPrefForKeyring(prefs_, kLegacyBraveWallet, base::Value(true),
                      keyring_id);
  else
    SetPrefForKeyring(prefs_, kLegacyBraveWallet, base::Value(false),
                      keyring_id);

  if (keyring_id == mojom::kDefaultKeyringId) {
    keyrings_[mojom::kDefaultKeyringId] = std::make_unique<HDKeyring>();
  } else if (IsFilecoinEnabled() && keyring_id == mojom::kFilecoinKeyringId) {
    keyrings_[mojom::kFilecoinKeyringId] = std::make_unique<FilecoinKeyring>();
  }
  auto* keyring = GetHDKeyringById(keyring_id);
  DCHECK(keyring) << "No HDKeyring for " << keyring_id;
  if (keyring)
    keyring->ConstructRootHDKey(*seed, kRootPath);

  UpdateLastUnlockPref(prefs_);

  return true;
}

bool KeyringService::IsKeyringCreated(const std::string& keyring_id) {
  return HasPrefForKeyring(prefs_, kEncryptedMnemonic, keyring_id);
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

void KeyringService::GetSelectedAccount(GetSelectedAccountCallback callback) {
  std::move(callback).Run(GetSelectedAccount());
}

void KeyringService::SetSelectedAccount(const std::string& address,
                                        SetSelectedAccountCallback callback) {
  std::vector<mojom::AccountInfoPtr> infos =
      GetAccountInfosForKeyring(mojom::kDefaultKeyringId);

  // Check for matching default and imported account
  for (const mojom::AccountInfoPtr& info : infos) {
    if (base::EqualsCaseInsensitiveASCII(info->address, address)) {
      prefs_->SetString(kBraveWalletSelectedAccount, address);
      std::move(callback).Run(true);
      return;
    }
  }

  auto hardware_account_info_ptrs = GetHardwareAccountsSync();
  for (const mojom::AccountInfoPtr& info : hardware_account_info_ptrs) {
    if (base::EqualsCaseInsensitiveASCII(info->address, address)) {
      prefs_->SetString(kBraveWalletSelectedAccount, address);
      std::move(callback).Run(true);
      return;
    }
  }

  std::move(callback).Run(false);
}

void KeyringService::SetKeyringDerivedAccountName(
    const std::string& keyring_id,
    const std::string& address,
    const std::string& name,
    KeyringService::SetKeyringDerivedAccountNameCallback callback) {
  auto* keyring = GetHDKeyringById(keyring_id);
  if (address.empty() || name.empty() || !keyring) {
    std::move(callback).Run(false);
    return;
  }

  const absl::optional<size_t> index = keyring->GetAccountIndex(address);
  if (!index) {
    std::move(callback).Run(false);
    return;
  }

  SetAccountMetaForKeyring(prefs_, GetAccountPathByIndex(index.value()), name,
                           address, keyring_id);
  NotifyAccountsChanged();
  std::move(callback).Run(true);
}

bool KeyringService::UpdateNameForHardwareAccountSync(
    const std::string& address,
    const std::string& name) {
  base::Value* hardware_keyrings = GetPrefForHardwareKeyringUpdate(prefs_);
  for (auto devices : hardware_keyrings->DictItems()) {
    base::Value* account_metas = devices.second.FindKey(kAccountMetas);
    if (!account_metas)
      continue;
    base::Value* address_key = account_metas->FindKey(address);
    if (!address_key)
      continue;
    address_key->SetStringKey(kAccountName, name);
    NotifyAccountsChanged();
    return true;
  }
  return false;
}

void KeyringService::SetDefaultKeyringHardwareAccountName(
    const std::string& address,
    const std::string& name,
    SetDefaultKeyringHardwareAccountNameCallback callback) {
  if (address.empty() || name.empty()) {
    std::move(callback).Run(false);
    return;
  }

  std::move(callback).Run(UpdateNameForHardwareAccountSync(address, name));
}

void KeyringService::SetKeyringImportedAccountName(
    const std::string& keyring_id,
    const std::string& address,
    const std::string& name,
    SetKeyringImportedAccountNameCallback callback) {
  auto* keyring = GetHDKeyringById(keyring_id);
  if (address.empty() || name.empty() || !keyring) {
    std::move(callback).Run(false);
    return;
  }

  base::Value imported_accounts(base::Value::Type::LIST);
  const base::Value* value =
      GetPrefForKeyring(prefs_, kImportedAccounts, keyring_id);
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
                        keyring_id);
      NotifyAccountsChanged();
      name_updated = true;
      break;
    }
  }

  std::move(callback).Run(name_updated);
}

void KeyringService::NotifyAccountsChanged() {
  for (const auto& observer : observers_) {
    observer->AccountsChanged();
  }
}

void KeyringService::OnAutoLockPreferenceChanged() {
  StopAutoLockTimer();
  ResetAutoLockTimer();
  for (const auto& observer : observers_) {
    observer->AutoLockMinutesChanged();
  }
}

void KeyringService::OnSelectedAccountPreferenceChanged() {
  for (const auto& observer : observers_) {
    observer->SelectedAccountChanged();
  }
}

void KeyringService::GetAutoLockMinutes(GetAutoLockMinutesCallback callback) {
  std::move(callback).Run(prefs_->GetInteger(kBraveWalletAutoLockMinutes));
}

void KeyringService::SetAutoLockMinutes(int32_t minutes,
                                        SetAutoLockMinutesCallback callback) {
  // Check bounds
  if (minutes < kAutoLockMinutesMin || minutes > kAutoLockMinutesMax) {
    std::move(callback).Run(false);
    return;
  }

  int32_t old_auto_lock_minutes =
      prefs_->GetInteger(kBraveWalletAutoLockMinutes);
  if (minutes != old_auto_lock_minutes) {
    prefs_->SetInteger(kBraveWalletAutoLockMinutes, minutes);
  }
  std::move(callback).Run(true);
}

void KeyringService::IsStrongPassword(const std::string& password,
                                      IsStrongPasswordCallback callback) {
  if (password.length() < 7) {
    std::move(callback).Run(false);
    return;
  }

  // Has at least one letter
  if (!RE2::PartialMatch(password, "[a-zA-Z]")) {
    std::move(callback).Run(false);
    return;
  }

  // Has at least one number
  if (!RE2::PartialMatch(password, "[0-9]")) {
    std::move(callback).Run(false);
    return;
  }

  // Has at least one non-alphanumeric character
  if (!RE2::PartialMatch(password, "[^0-9a-zA-Z]")) {
    std::move(callback).Run(false);
    return;
  }

  std::move(callback).Run(true);
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

}  // namespace brave_wallet
