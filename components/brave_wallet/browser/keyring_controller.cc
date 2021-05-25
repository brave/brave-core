/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/browser/keyring_controller.h"

#include "base/base64.h"
#include "base/logging.h"
#include "brave/components/brave_wallet/browser/brave_wallet_utils.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/pref_names.h"
#include "components/prefs/pref_service.h"
#include "crypto/random.h"

namespace brave_wallet {
namespace {
const size_t kSaltSize = 32;
const size_t kNonceSize = 12;

static base::span<const uint8_t> ToSpan(base::StringPiece sp) {
  return base::as_bytes(base::make_span(sp));
}
}  // namespace

KeyringController::KeyringController(PrefService* prefs) : prefs_(prefs) {
  DCHECK(prefs);
}

KeyringController::~KeyringController() {
  // Store the accounts number for keyring resume
  if (!IsLocked() && default_keyring_)
    prefs_->SetInteger(kBraveWalletDefaultKeyringAccountNum,
                       default_keyring_->GetAccounts().size());
}

HDKeyring* KeyringController::CreateDefaultKeyring(
    const std::string& password) {
  if (!CreateEncryptor(password))
    return nullptr;

  const std::string mnemonic = GenerateMnemonic(16);
  if (!CreateDefaultKeyringInternal(mnemonic)) {
    return nullptr;
  }

  return default_keyring_.get();
}

HDKeyring* KeyringController::ResumeDefaultKeyring(
    const std::string& password) {
  if (!CreateEncryptor(password)) {
    return nullptr;
  }

  const std::string mnemonic = GetMnemonicForDefaultKeyring();
  if (mnemonic.empty() || !CreateDefaultKeyringInternal(mnemonic)) {
    return nullptr;
  }
  size_t account_no =
      (size_t)prefs_->GetInteger(kBraveWalletDefaultKeyringAccountNum);
  if (account_no)
    default_keyring_->AddAccounts(account_no);

  return default_keyring_.get();
}

HDKeyring* KeyringController::RestoreDefaultKeyring(
    const std::string& mnemonic,
    const std::string& password) {
  Reset();

  if (!CreateEncryptor(password))
    return nullptr;

  if (!CreateDefaultKeyringInternal(mnemonic)) {
    return nullptr;
  }

  return default_keyring_.get();
}

std::string KeyringController::GetMnemonicForDefaultKeyring() {
  if (IsLocked()) {
    LOG(ERROR) << __func__ << ": Must Unlock controller first";
    return std::string();
  }
  DCHECK(encryptor_);
  std::vector<uint8_t> encrypted_mnemonic;
  if (!GetPrefsInBytes(kBraveWalletEncryptedMnemonic, &encrypted_mnemonic)) {
    return std::string();
  }
  std::vector<uint8_t> mnemonic;
  if (!encryptor_->Decrypt(encrypted_mnemonic, GetOrCreateNonce(), &mnemonic)) {
    return std::string();
  }

  return std::string(mnemonic.begin(), mnemonic.end());
}

HDKeyring* KeyringController::GetDefaultKeyring() {
  if (IsLocked()) {
    LOG(ERROR) << __func__ << ": Must Unlock controller first";
    return nullptr;
  }

  return default_keyring_.get();
}

bool KeyringController::IsLocked() const {
  return encryptor_ == nullptr;
}

void KeyringController::Lock() {
  if (IsLocked() || !default_keyring_)
    return;
  // invalidate keyring and save account number
  prefs_->SetInteger(kBraveWalletDefaultKeyringAccountNum,
                     default_keyring_->GetAccounts().size());
  default_keyring_->ClearData();

  encryptor_.reset();
}

bool KeyringController::Unlock(const std::string& password) {
  if (!ResumeDefaultKeyring(password)) {
    encryptor_.reset();
    return false;
  }

  return true;
}

void KeyringController::Reset() {
  prefs_->ClearPref(kBraveWalletPasswordEncryptorSalt);
  prefs_->ClearPref(kBraveWalletPasswordEncryptorNonce);
  encryptor_.reset();

  default_keyring_.reset();
  prefs_->ClearPref(kBraveWalletEncryptedMnemonic);
  prefs_->ClearPref(kBraveWalletDefaultKeyringAccountNum);
}

bool KeyringController::GetPrefsInBytes(const std::string& path,
                                        std::vector<uint8_t>* bytes) {
  if (!bytes || !prefs_->HasPrefPath(path)) {
    return false;
  }
  const std::string encoded = prefs_->GetString(path);
  if (encoded.empty())
    return false;

  std::string decoded;
  if (!base::Base64Decode(encoded, &decoded)) {
    return false;
  }
  *bytes = std::vector<uint8_t>(decoded.begin(), decoded.end());
  return true;
}

void KeyringController::SetPrefsInBytes(const std::string& path,
                                        base::span<const uint8_t> bytes) {
  const std::string encoded = base::Base64Encode(bytes);
  prefs_->SetString(path, encoded);
}

std::vector<uint8_t> KeyringController::GetOrCreateNonce() {
  std::vector<uint8_t> nonce(kNonceSize);
  if (!GetPrefsInBytes(kBraveWalletPasswordEncryptorNonce, &nonce)) {
    crypto::RandBytes(nonce);
    SetPrefsInBytes(kBraveWalletPasswordEncryptorNonce, nonce);
  }
  return nonce;
}

bool KeyringController::CreateEncryptor(const std::string& password) {
  if (password.empty())
    return false;
  std::vector<uint8_t> salt(kSaltSize);
  if (!GetPrefsInBytes(kBraveWalletPasswordEncryptorSalt, &salt)) {
    crypto::RandBytes(salt);
    SetPrefsInBytes(kBraveWalletPasswordEncryptorSalt, salt);
  }
  encryptor_ = PasswordEncryptor::DeriveKeyFromPasswordUsingPbkdf2(
      password, salt, 100000, 256);
  return encryptor_ != nullptr;
}

bool KeyringController::CreateDefaultKeyringInternal(
    const std::string& mnemonic) {
  if (!encryptor_)
    return false;
  std::vector<uint8_t> encrypted_mnemonic;
  if (!encryptor_->Encrypt(ToSpan(mnemonic), GetOrCreateNonce(),
                           &encrypted_mnemonic)) {
    return false;
  }
  SetPrefsInBytes(kBraveWalletEncryptedMnemonic, encrypted_mnemonic);

  const std::unique_ptr<std::vector<uint8_t>> seed =
      MnemonicToSeed(mnemonic, "");
  default_keyring_ = std::make_unique<HDKeyring>();
  default_keyring_->ConstructRootHDKey(*seed, "m/44'/60'/0'/0");

  return true;
}

bool KeyringController::IsDefaultKeyringCreated() {
  return prefs_->HasPrefPath(kBraveWalletEncryptedMnemonic);
}

}  // namespace brave_wallet
