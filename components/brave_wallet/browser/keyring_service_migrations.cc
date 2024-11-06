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
