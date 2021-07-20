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
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}

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
class KeyringController : public KeyedService, public mojom::KeyringController {
 public:
  explicit KeyringController(PrefService* prefs);
  ~KeyringController() override;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* prefs);

  mojo::PendingRemote<mojom::KeyringController> MakeRemote();

  // Currently only support one default keyring, `CreateDefaultKeyring` and
  // `RestoreDefaultKeyring` will overwrite existing one if success
  HDKeyring* CreateDefaultKeyring(const std::string& password);
  // Restore default keyring from backup seed phrase
  HDKeyring* RestoreDefaultKeyring(const std::string& mnemonic,
                                   const std::string& password);

  // mojom::KeyringController
  // Must unlock before using this API otherwise it will return empty string
  void GetMnemonicForDefaultKeyring(
      GetMnemonicForDefaultKeyringCallback callback) override;
  void CreateWallet(const std::string& password,
                    CreateWalletCallback callback) override;
  void RestoreWallet(const std::string& mnemonic,
                     const std::string& password,
                     RestoreWalletCallback callback) override;
  void Unlock(const std::string& password, UnlockCallback callback) override;
  void Lock() override;
  void AddAccount(AddAccountCallback callback) override;
  void IsWalletBackedUp(IsWalletBackedUpCallback callback) override;
  void NotifyWalletBackupComplete() override;
  void SetInitialAccountNames(
      const std::vector<std::string>& account_names) override;
  void AddNewAccountName(const std::string& account_name) override;
  void GetDefaultKeyringInfo(GetDefaultKeyringInfoCallback callback) override;
  void Reset() override;

  // Must unlock before using this API otherwise it will return nullptr
  HDKeyring* GetDefaultKeyring();
  bool IsDefaultKeyringCreated();

  bool IsLocked() const;
  // bool Unlock(const std::string& password);

  /* TODO(darkdh): For other keyrings support
  void DeleteKeyring(size_t index);
  HDKeyring* GetKeyring(size_t index);
  */

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

  std::vector<std::string> GetAccountNames() const;

  const std::string GetMnemonicForDefaultKeyringImpl();

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

  mojo::ReceiverSet<mojom::KeyringController> receivers_;

  KeyringController(const KeyringController&) = delete;
  KeyringController& operator=(const KeyringController&) = delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_CONTROLLER_H_
