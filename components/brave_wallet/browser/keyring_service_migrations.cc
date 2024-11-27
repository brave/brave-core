/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/keyring_service_migrations.h"

#include <map>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/base64.h"
#include "base/check_op.h"
#include "base/containers/span.h"
#include "base/notreached.h"
#include "base/strings/string_util.h"
#include "base/value_iterators.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_prefs.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/keyring_service.h"
#include "brave/components/brave_wallet/browser/keyring_service_prefs.h"
#include "brave/components/brave_wallet/browser/password_encryptor.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/common_utils.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "crypto/random.h"

namespace brave_wallet {

namespace {

constexpr int kPbkdf2IterationsLegacy = 100000;
constexpr char kBackupCompleteDeprecated[] = "backup_complete";
constexpr char kLegacyBraveWalletDeprecated[] = "legacy_brave_wallet";
constexpr char kPasswordEncryptorSaltDeprecated[] = "password_encryptor_salt";
constexpr char kPasswordEncryptorNonceDeprecated[] = "password_encryptor_nonce";
constexpr char kEncryptedMnemonicDeprecated[] = "encrypted_mnemonic";
constexpr char kImportedAccountCoinTypeDeprecated[] = "coin_type";
constexpr char kSelectedAccountDeprecated[] = "selected_account";

std::optional<uint32_t> ExtractAccountIndex(mojom::KeyringId keyring_id,
                                            const std::string& path) {
  CHECK(keyring_id == mojom::KeyringId::kDefault ||
        keyring_id == mojom::KeyringId::kFilecoin ||
        keyring_id == mojom::KeyringId::kFilecoinTestnet ||
        keyring_id == mojom::KeyringId::kSolana);

  // m/44'/60'/0'/0/{index}
  // m/44'/461'/0'/0/{index}
  // m/44'/1'/0'/0/{index}
  // m/44'/501'/{index}'/0'

  // For all types remove root path and slash. For Solana also remove '/0'.

  auto account_index = std::string_view(path);
  auto root_path = HDKeyring::GetRootPath(keyring_id);
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

std::optional<std::vector<uint8_t>> GetPrefInBytesForKeyringDeprecated(
    PrefService* profile_prefs,
    const std::string& key,
    mojom::KeyringId keyring_id) {
  const base::Value* value = GetPrefForKeyring(profile_prefs, key, keyring_id);
  if (!value) {
    return std::nullopt;
  }

  const std::string* encoded = value->GetIfString();
  if (!encoded || encoded->empty()) {
    return std::nullopt;
  }

  return base::Base64Decode(*encoded);
}

std::vector<uint8_t> GetOrCreateNonceForKeyringDeprecated(
    PrefService* profile_prefs,
    mojom::KeyringId keyring_id,
    bool force_create) {
  if (!force_create) {
    if (auto nonce = GetPrefInBytesForKeyringDeprecated(
            profile_prefs, kPasswordEncryptorNonceDeprecated, keyring_id)) {
      return *nonce;
    }
  }

  std::vector<uint8_t> nonce(kEncryptorNonceSize);
  crypto::RandBytes(nonce);
  SetPrefForKeyring(profile_prefs, kPasswordEncryptorNonceDeprecated,
                    base::Value(base::Base64Encode(nonce)), keyring_id);
  return nonce;
}

std::vector<uint8_t> GetOrCreateSaltForKeyringDeprecated(
    PrefService* profile_prefs,
    mojom::KeyringId keyring_id,
    bool force_create) {
  if (!force_create) {
    if (auto salt = GetPrefInBytesForKeyringDeprecated(
            profile_prefs, kPasswordEncryptorSaltDeprecated, keyring_id)) {
      return *salt;
    }
  }

  std::vector<uint8_t> salt(kEncryptorSaltSize);
  crypto::RandBytes(salt);
  SetPrefForKeyring(profile_prefs, kPasswordEncryptorSaltDeprecated,
                    base::Value(base::Base64Encode(salt)), keyring_id);
  return salt;
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

void MaybeMigrateSelectedAccountPrefs(
    PrefService* profile_prefs,
    const std::vector<mojom::AccountInfoPtr>& all_accounts) {
  if (!profile_prefs->HasPrefPath(kBraveWalletSelectedCoinDeprecated)) {
    return;
  }

  if (all_accounts.empty()) {
    return;
  }

  auto find_account = [&](mojom::KeyringId keyring_id) -> mojom::AccountIdPtr {
    if (!profile_prefs->GetDict(kBraveWalletKeyrings)
             .FindDict(KeyringIdPrefString(keyring_id))) {
      return nullptr;
    }

    std::string address;
    {
      ScopedDictPrefUpdate update(profile_prefs, kBraveWalletKeyrings);
      auto extracted = update->EnsureDict(KeyringIdPrefString(keyring_id))
                           ->Extract(kSelectedAccountDeprecated);
      if (extracted && extracted->is_string()) {
        address = extracted->GetString();
      }
    }

    if (address.empty()) {
      return nullptr;
    }

    for (auto& acc : all_accounts) {
      if (acc->account_id->keyring_id == keyring_id &&
          base::EqualsCaseInsensitiveASCII(acc->address, address)) {
        return acc->account_id.Clone();
      }
    }

    return nullptr;
  };

  mojom::AccountIdPtr eth_selected = find_account(mojom::KeyringId::kDefault);
  mojom::AccountIdPtr sol_selected = find_account(mojom::KeyringId::kSolana);
  mojom::AccountIdPtr fil_selected = find_account(mojom::KeyringId::kFilecoin);

  SetSelectedDappAccountInPrefs(profile_prefs, mojom::CoinType::ETH,
                                eth_selected ? eth_selected->unique_key : "");
  SetSelectedDappAccountInPrefs(profile_prefs, mojom::CoinType::SOL,
                                sol_selected ? sol_selected->unique_key : "");

  mojom::AccountIdPtr wallet_selected;
  auto coin = static_cast<mojom::CoinType>(
      profile_prefs->GetInteger(kBraveWalletSelectedCoinDeprecated));
  switch (coin) {
    case mojom::CoinType::ETH:
      wallet_selected = eth_selected.Clone();
      break;
    case mojom::CoinType::SOL:
      wallet_selected = sol_selected.Clone();
      break;
    case mojom::CoinType::FIL:
      wallet_selected = fil_selected.Clone();
      break;
    case mojom::CoinType::ZEC:
      NOTREACHED();
    case mojom::CoinType::BTC:
      NOTREACHED();
  }

  if (!wallet_selected) {
    wallet_selected = all_accounts.front()->account_id->Clone();
    DCHECK_EQ(mojom::CoinType::ETH, wallet_selected->coin);
  }
  SetSelectedWalletAccountInPrefs(profile_prefs, wallet_selected->unique_key);
  profile_prefs->ClearPref(kBraveWalletSelectedCoinDeprecated);
}

void MaybeRunPasswordMigrations(PrefService* profile_prefs,
                                const std::string& password) {
  MaybeMigratePBKDF2Iterations(profile_prefs, password);
  MaybeMigrateToWalletMnemonic(profile_prefs, password);
}

void MaybeMigratePBKDF2Iterations(PrefService* profile_prefs,
                                  const std::string& password) {
  if (profile_prefs->GetBoolean(kBraveWalletKeyringEncryptionKeysMigrated)) {
    return;
  }

  // Pref is supposed to be set only as true.
  DCHECK(
      !profile_prefs->HasPrefPath(kBraveWalletKeyringEncryptionKeysMigrated));

  for (auto keyring_id :
       {mojom::kDefaultKeyringId, mojom::kFilecoinKeyringId,
        mojom::kFilecoinTestnetKeyringId, mojom::kSolanaKeyringId}) {
    auto deprecated_encrypted_mnemonic = GetPrefInBytesForKeyringDeprecated(
        profile_prefs, kEncryptedMnemonicDeprecated, keyring_id);
    auto deprecated_nonce = GetPrefInBytesForKeyringDeprecated(
        profile_prefs, kPasswordEncryptorNonceDeprecated, keyring_id);
    auto deprecated_salt = GetPrefInBytesForKeyringDeprecated(
        profile_prefs, kPasswordEncryptorSaltDeprecated, keyring_id);

    if (!deprecated_encrypted_mnemonic || !deprecated_nonce ||
        !deprecated_salt) {
      continue;
    }

    auto deprecated_encryptor =
        PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
            password, *deprecated_salt, kPbkdf2IterationsLegacy,
            kPbkdf2KeySize);
    if (!deprecated_encryptor) {
      continue;
    }

    auto mnemonic = deprecated_encryptor->Decrypt(
        *deprecated_encrypted_mnemonic, *deprecated_nonce);
    if (!mnemonic) {
      continue;
    }

    auto salt = GetOrCreateSaltForKeyringDeprecated(profile_prefs, keyring_id,
                                                    /*force_create = */ true);

    auto encryptor = PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
        password, salt, kPbkdf2Iterations, kPbkdf2KeySize);
    if (!encryptor) {
      continue;
    }

    auto nonce = GetOrCreateNonceForKeyringDeprecated(profile_prefs, keyring_id,
                                                      /*force_create = */ true);

    SetPrefForKeyring(profile_prefs, kEncryptedMnemonicDeprecated,
                      base::Value(base::Base64Encode(
                          encryptor->Encrypt(base::span(*mnemonic), nonce))),
                      keyring_id);

    if (keyring_id == mojom::kDefaultKeyringId) {
      profile_prefs->SetBoolean(kBraveWalletKeyringEncryptionKeysMigrated,
                                true);
    }

    const base::Value* deprecated_imported_accounts =
        GetPrefForKeyring(profile_prefs, kImportedAccounts, keyring_id);
    if (!deprecated_imported_accounts ||
        !deprecated_imported_accounts->is_list()) {
      continue;
    }
    base::Value::List imported_accounts =
        deprecated_imported_accounts->GetList().Clone();
    for (auto& imported_account : imported_accounts) {
      if (!imported_account.is_dict()) {
        continue;
      }

      const std::string* deprecated_encrypted_private_key =
          imported_account.GetDict().FindString(kEncryptedPrivateKey);
      if (!deprecated_encrypted_private_key) {
        continue;
      }

      auto deprecated_private_key_decoded =
          base::Base64Decode(*deprecated_encrypted_private_key);
      if (!deprecated_private_key_decoded) {
        continue;
      }

      auto private_key = deprecated_encryptor->Decrypt(
          base::span(*deprecated_private_key_decoded), *deprecated_nonce);
      if (!private_key) {
        continue;
      }

      imported_account.GetDict().Set(
          kEncryptedPrivateKey,
          base::Base64Encode(encryptor->Encrypt(*private_key, nonce)));
    }
    SetPrefForKeyring(profile_prefs, kImportedAccounts,
                      base::Value(std::move(imported_accounts)), keyring_id);
  }
}

void MaybeMigrateToWalletMnemonic(PrefService* profile_prefs,
                                  const std::string& password) {
  auto deprecated_eth_encrypted_mnemonic = GetPrefInBytesForKeyringDeprecated(
      profile_prefs, kEncryptedMnemonicDeprecated, mojom::KeyringId::kDefault);
  if (!deprecated_eth_encrypted_mnemonic) {
    return;
  }

  auto deprecated_eth_nonce = GetPrefInBytesForKeyringDeprecated(
      profile_prefs, kPasswordEncryptorNonceDeprecated,
      mojom::KeyringId::kDefault);
  auto deprecated_eth_salt = GetPrefInBytesForKeyringDeprecated(
      profile_prefs, kPasswordEncryptorSaltDeprecated,
      mojom::KeyringId::kDefault);
  if (!deprecated_eth_nonce || !deprecated_eth_salt) {
    return;
  }

  auto deprecated_eth_encryptor =
      PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
          password, *deprecated_eth_salt, kPbkdf2Iterations, kPbkdf2KeySize);
  if (!deprecated_eth_encryptor) {
    return;
  }

  auto mnemonic = deprecated_eth_encryptor->Decrypt(
      *deprecated_eth_encrypted_mnemonic, *deprecated_eth_nonce);
  if (!mnemonic) {
    return;
  }

  auto wallet_salt = PasswordEncryptor::CreateSalt();
  auto wallet_encryptor =
      PasswordEncryptor::CreateEncryptor(password, wallet_salt);
  if (!wallet_encryptor) {
    return;
  }

  if (auto* value =
          GetPrefForKeyring(profile_prefs, kLegacyBraveWalletDeprecated,
                            mojom::KeyringId::kDefault)) {
    if (value->GetIfBool().value_or(false)) {
      profile_prefs->SetBoolean(kBraveWalletLegacyEthSeedFormat, true);
    }
  }

  if (auto* value = GetPrefForKeyring(profile_prefs, kBackupCompleteDeprecated,
                                      mojom::KeyringId::kDefault)) {
    profile_prefs->SetBoolean(kBraveWalletMnemonicBackedUp,
                              value->GetIfBool().value_or(false));
  }

  profile_prefs->SetString(kBraveWalletEncryptorSalt,
                           base::Base64Encode(wallet_salt));
  profile_prefs->SetDict(
      kBraveWalletMnemonic,
      wallet_encryptor->EncryptToDict(base::as_byte_span(*mnemonic),
                                      PasswordEncryptor::CreateNonce()));

  for (auto keyring_id :
       {mojom::KeyringId::kDefault, mojom::KeyringId::kFilecoin,
        mojom::KeyringId::kFilecoinTestnet, mojom::KeyringId::kSolana,
        mojom::KeyringId::kBitcoin84, mojom::KeyringId::kBitcoin84Testnet,
        mojom::KeyringId::kZCashMainnet, mojom::KeyringId::kZCashTestnet}) {
    auto deprecated_encrypted_mnemonic = GetPrefInBytesForKeyringDeprecated(
        profile_prefs, kEncryptedMnemonicDeprecated, keyring_id);
    auto deprecated_nonce = GetPrefInBytesForKeyringDeprecated(
        profile_prefs, kPasswordEncryptorNonceDeprecated, keyring_id);
    auto deprecated_salt = GetPrefInBytesForKeyringDeprecated(
        profile_prefs, kPasswordEncryptorSaltDeprecated, keyring_id);

    SetPrefForKeyring(profile_prefs, kEncryptedMnemonicDeprecated,
                      base::Value(), keyring_id);
    SetPrefForKeyring(profile_prefs, kPasswordEncryptorNonceDeprecated,
                      base::Value(), keyring_id);
    SetPrefForKeyring(profile_prefs, kPasswordEncryptorSaltDeprecated,
                      base::Value(), keyring_id);
    SetPrefForKeyring(profile_prefs, kLegacyBraveWalletDeprecated,
                      base::Value(), keyring_id);
    SetPrefForKeyring(profile_prefs, kSelectedAccountDeprecated, base::Value(),
                      keyring_id);
    SetPrefForKeyring(profile_prefs, kBackupCompleteDeprecated, base::Value(),
                      keyring_id);

    if (!deprecated_encrypted_mnemonic || !deprecated_nonce ||
        !deprecated_salt) {
      continue;
    }

    auto deprecated_encryptor =
        PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
            password, *deprecated_salt, kPbkdf2Iterations, kPbkdf2KeySize);
    if (!deprecated_encryptor) {
      continue;
    }

    const base::Value* deprecated_imported_accounts =
        GetPrefForKeyring(profile_prefs, kImportedAccounts, keyring_id);
    if (!deprecated_imported_accounts ||
        !deprecated_imported_accounts->is_list()) {
      continue;
    }
    base::Value::List imported_accounts =
        deprecated_imported_accounts->GetList().Clone();
    for (auto& imported_account : imported_accounts) {
      if (!imported_account.is_dict()) {
        continue;
      }

      const std::string* deprecated_encrypted_private_key =
          imported_account.GetDict().FindString(kEncryptedPrivateKey);
      if (!deprecated_encrypted_private_key) {
        continue;
      }

      auto deprecated_private_key_decoded =
          base::Base64Decode(*deprecated_encrypted_private_key);
      if (!deprecated_private_key_decoded) {
        continue;
      }

      auto private_key = deprecated_encryptor->Decrypt(
          base::span(*deprecated_private_key_decoded), *deprecated_nonce);
      if (!private_key) {
        continue;
      }

      imported_account.GetDict().Set(
          kEncryptedPrivateKey,
          wallet_encryptor->EncryptToDict(*private_key,
                                          PasswordEncryptor::CreateNonce()));

      imported_account.GetDict().Remove(kImportedAccountCoinTypeDeprecated);
    }
    SetPrefForKeyring(profile_prefs, kImportedAccounts,
                      base::Value(std::move(imported_accounts)), keyring_id);
  }
}

}  // namespace brave_wallet
