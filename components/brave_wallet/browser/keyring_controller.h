/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_CONTROLLER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "brave/components/brave_wallet/browser/password_encryptor.h"

class PrefService;

namespace brave_wallet {

class HDKeyring;
class KeyringControllerUnitTest;

FORWARD_DECLARE_TEST(KeyringControllerUnitTest, GetPrefsInBytes);
FORWARD_DECLARE_TEST(KeyringControllerUnitTest, SetPrefsInBytes);
FORWARD_DECLARE_TEST(KeyringControllerUnitTest, GetOrCreateNonce);
FORWARD_DECLARE_TEST(KeyringControllerUnitTest, CreateEncryptor);
FORWARD_DECLARE_TEST(KeyringControllerUnitTest, CreateDefaultKeyringInternal);
FORWARD_DECLARE_TEST(KeyringControllerUnitTest, GetMnemonicForDefaultKeyring);
FORWARD_DECLARE_TEST(KeyringControllerUnitTest, LockAndUnlock);
FORWARD_DECLARE_TEST(KeyringControllerUnitTest, Reset);

// This class is not thread-safe and should have single owner
class KeyringController {
 public:
  explicit KeyringController(PrefService* prefs);
  ~KeyringController();

  // Currently only support one default keyring, `CreateDefaultKeyring` and
  // `RestoreDefaultKeyring` will overwrite existing one if success
  HDKeyring* CreateDefaultKeyring(const std::string& password);
  // Restore default keyring from backup seed phrase
  HDKeyring* RestoreDefaultKeyring(const std::string& mnemonic,
                                   const std::string& password);
  // Must unlock before using this API otherwise it will return empty string
  std::string GetMnemonicForDefaultKeyring();
  // Must unlock before using this API otherwise it will return nullptr
  HDKeyring* GetDefaultKeyring();
  bool IsDefaultKeyringCreated();

  bool IsLocked() const;
  void Lock();
  bool Unlock(const std::string& password);

  /* TODO(darkdh): For other keyrings support
  void DeleteKeyring(size_t index);
  HDKeyring* GetKeyring(size_t index);
  */

  void Reset();

 private:
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest, GetPrefsInBytes);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest, SetPrefsInBytes);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest, GetOrCreateNonce);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest, CreateEncryptor);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest,
                           CreateDefaultKeyringInternal);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest,
                           GetMnemonicForDefaultKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest, LockAndUnlock);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest, Reset);

  bool GetPrefsInBytes(const std::string& path, std::vector<uint8_t>* bytes);
  void SetPrefsInBytes(const std::string& path,
                       base::span<const uint8_t> bytes);
  std::vector<uint8_t> GetOrCreateNonce();
  bool CreateEncryptor(const std::string& password);
  bool CreateDefaultKeyringInternal(const std::string& mnemonic);
  // It's used to reconstruct same default keyring between browser relaunch
  HDKeyring* ResumeDefaultKeyring(const std::string& password);

  std::unique_ptr<PasswordEncryptor> encryptor_;
  std::unique_ptr<HDKeyring> default_keyring_;

  // TODO(darkdh): For other keyrings support
  // std::vector<std::unique_ptr<HDKeyring>> keyrings_;

  PrefService* prefs_;

  KeyringController(const KeyringController&) = delete;
  KeyringController& operator=(const KeyringController&) = delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_CONTROLLER_H_
