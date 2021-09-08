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
#include "base/values.h"
#include "brave/components/brave_wallet/browser/brave_wallet_types.h"
#include "brave/components/brave_wallet/browser/password_encryptor.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefService;

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace brave_wallet {

class HDKeyring;
class EthTransaction;
class KeyringControllerUnitTest;

// This class is not thread-safe and should have single owner
class KeyringController : public KeyedService, public mojom::KeyringController {
 public:
  explicit KeyringController(PrefService* prefs);
  ~KeyringController() override;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  static void RegisterProfilePrefsForMigration(
      user_prefs::PrefRegistrySyncable* registry);

  static void MigrateObsoleteProfilePrefs(PrefService* prefs);

  static bool HasPrefForKeyring(PrefService* prefs,
                                const std::string& key,
                                const std::string& id);
  static const base::Value* GetPrefForKeyring(PrefService* prefs,
                                              const std::string& key,
                                              const std::string& id);
  // If keyring dicionary for id doesn't exist, it will be created.
  static void SetPrefForKeyring(PrefService* prefs,
                                const std::string& key,
                                base::Value value,
                                const std::string& id);

  // Account path will be used as key in kAccountMetas
  static void SetAccountNameForKeyring(PrefService* prefs,
                                       const std::string& account_path,
                                       const std::string& name,
                                       const std::string& id);
  static std::string GetAccountNameForKeyring(PrefService* prefs,
                                              const std::string& account_path,
                                              const std::string& id);

  static std::string GetAccountPathByIndex(size_t index);

  struct ImportedAccountInfo {
    std::string account_name;
    std::string account_address;
    std::string encrypted_private_key;
  };
  static void SetImportedAccountForKeyring(PrefService* prefs,
                                           const ImportedAccountInfo& info,
                                           const std::string& id);
  static std::vector<ImportedAccountInfo> GetImportedAccountsForKeyring(
      PrefService* prefs,
      const std::string& id);
  static void RemoveImportedAccountForKeyring(PrefService* prefs,
                                              const std::string& address,
                                              const std::string& id);

  mojo::PendingRemote<mojom::KeyringController> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::KeyringController> receiver);

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
  void IsLocked(IsLockedCallback callback) override;
  void AddAccount(const std::string& account_name,
                  AddAccountCallback callback) override;
  void GetPrivateKeyForDefaultKeyringAccount(
      const std::string& address,
      GetPrivateKeyForDefaultKeyringAccountCallback callback) override;
  void ImportAccount(const std::string& account_name,
                     const std::string& private_key,
                     ImportAccountCallback callback) override;
  void ImportAccountFromJson(const std::string& account_name,
                             const std::string& password,
                             const std::string& json,
                             ImportAccountCallback callback) override;
  void GetPrivateKeyForImportedAccount(
      const std::string& address,
      GetPrivateKeyForImportedAccountCallback callback) override;
  void RemoveImportedAccount(const std::string& address,
                             RemoveImportedAccountCallback callback) override;
  void IsWalletBackedUp(IsWalletBackedUpCallback callback) override;
  void NotifyWalletBackupComplete() override;
  void GetDefaultKeyringInfo(GetDefaultKeyringInfoCallback callback) override;
  void Reset() override;
  void SetDefaultKeyringDerivedAccountName(
      const std::string& address,
      const std::string& name,
      SetDefaultKeyringDerivedAccountNameCallback callback) override;
  void SetDefaultKeyringImportedAccountName(
      const std::string& address,
      const std::string& name,
      SetDefaultKeyringImportedAccountNameCallback callback) override;

  bool IsDefaultKeyringCreated();

  void SignTransactionByDefaultKeyring(const std::string& address,
                                       EthTransaction* tx,
                                       uint256_t chain_id);

  bool IsLocked() const;

  void AddObserver(::mojo::PendingRemote<mojom::KeyringControllerObserver>
                       observer) override;

  /* TODO(darkdh): For other keyrings support
  void DeleteKeyring(size_t index);
  HDKeyring* GetKeyring(size_t index);
  */

 private:
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest, GetPrefInBytesForKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest, SetPrefInBytesForKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest,
                           GetOrCreateNonceForKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest,
                           CreateEncryptorForKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest, CreateDefaultKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest,
                           CreateDefaultKeyringInternal);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest, RestoreDefaultKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest,
                           UnlockResumesDefaultKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest,
                           GetMnemonicForDefaultKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest, LockAndUnlock);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest, Reset);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest, AccountMetasForKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest, CreateAndRestoreWallet);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest, AddAccount);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest, ImportedAccounts);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest,
                           GetPrivateKeyForDefaultKeyringAccount);
  FRIEND_TEST_ALL_PREFIXES(KeyringControllerUnitTest,
                           SetDefaultKeyringDerivedAccountName);

  void AddAccountForDefaultKeyring(const std::string& account_name);

  // Address will be returned when success
  absl::optional<std::string> ImportAccountForDefaultKeyring(
      const std::string& account_name,
      const std::vector<uint8_t>& private_key);

  size_t GetAccountMetasNumberForKeyring(const std::string& id);

  std::vector<mojom::AccountInfoPtr> GetAccountInfosForKeyring(
      const std::string& id);

  const std::string GetMnemonicForDefaultKeyringImpl();

  bool GetPrefInBytesForKeyring(const std::string& key,
                                std::vector<uint8_t>* bytes,
                                const std::string& id) const;
  void SetPrefInBytesForKeyring(const std::string& key,
                                base::span<const uint8_t> bytes,
                                const std::string& id);
  std::vector<uint8_t> GetOrCreateNonceForKeyring(const std::string& id);
  bool CreateEncryptorForKeyring(const std::string& password,
                                 const std::string& id);
  bool CreateDefaultKeyringInternal(const std::string& mnemonic);

  // Currently only support one default keyring, `CreateDefaultKeyring` and
  // `RestoreDefaultKeyring` will overwrite existing one if success
  HDKeyring* CreateDefaultKeyring(const std::string& password);
  // Restore default keyring from backup seed phrase
  HDKeyring* RestoreDefaultKeyring(const std::string& mnemonic,
                                   const std::string& password);
  // It's used to reconstruct same default keyring between browser relaunch
  HDKeyring* ResumeDefaultKeyring(const std::string& password);

  void NotifyAccountsChanged();

  std::unique_ptr<PasswordEncryptor> encryptor_;
  std::unique_ptr<HDKeyring> default_keyring_;

  // TODO(darkdh): For other keyrings support
  // std::vector<std::unique_ptr<HDKeyring>> keyrings_;

  PrefService* prefs_;

  mojo::RemoteSet<mojom::KeyringControllerObserver> observers_;
  mojo::ReceiverSet<mojom::KeyringController> receivers_;

  KeyringController(const KeyringController&) = delete;
  KeyringController& operator=(const KeyringController&) = delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_CONTROLLER_H_
