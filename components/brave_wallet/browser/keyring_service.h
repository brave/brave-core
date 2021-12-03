/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/password_encryptor.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

class PrefChangeRegistrar;
class PrefService;

namespace base {
class OneShotTimer;
}

namespace brave_wallet {

class EthTransaction;
class KeyringServiceUnitTest;
class BraveWalletProviderImplUnitTest;
class FilecoinKeyring;

// This class is not thread-safe and should have single owner
class KeyringService : public KeyedService, public mojom::KeyringService {
 public:
  explicit KeyringService(PrefService* prefs);
  ~KeyringService() override;

  static void MigrateObsoleteProfilePrefs(PrefService* prefs);

  static bool HasPrefForKeyring(PrefService* prefs,
                                const std::string& key,
                                const std::string& id);
  static const base::Value* GetPrefForKeyring(PrefService* prefs,
                                              const std::string& key,
                                              const std::string& id);
  static base::Value* GetPrefForHardwareKeyringUpdate(PrefService* prefs);
  static base::Value* GetPrefForKeyringUpdate(PrefService* prefs,
                                              const std::string& key,
                                              const std::string& id);
  // If keyring dicionary for id doesn't exist, it will be created.
  static void SetPrefForKeyring(PrefService* prefs,
                                const std::string& key,
                                base::Value value,
                                const std::string& id);

  // Account path will be used as key in kAccountMetas
  static void SetAccountMetaForKeyring(
      PrefService* prefs,
      const std::string& account_path,
      const absl::optional<std::string> name,
      const absl::optional<std::string> address,
      const std::string& id);

  static std::string GetAccountNameForKeyring(PrefService* prefs,
                                              const std::string& account_path,
                                              const std::string& id);
  static std::string GetAccountAddressForKeyring(
      PrefService* prefs,
      const std::string& account_path,
      const std::string& id);

  static std::string GetAccountPathByIndex(size_t index);

  struct ImportedAccountInfo {
    ImportedAccountInfo(const std::string& account_name,
                        const std::string& account_address,
                        const std::string& encrypted_private_key,
                        mojom::CoinType coin);
    ~ImportedAccountInfo();
    ImportedAccountInfo(const ImportedAccountInfo& other);
    std::string account_name;
    std::string account_address;
    std::string encrypted_private_key;
    mojom::CoinType coin;
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

  mojo::PendingRemote<mojom::KeyringService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::KeyringService> receiver);

  // mojom::KeyringService
  // Must unlock before using this API otherwise it will return empty string
  void GetMnemonicForDefaultKeyring(
      GetMnemonicForDefaultKeyringCallback callback) override;
  void CreateWallet(const std::string& password,
                    CreateWalletCallback callback) override;
  void RestoreWallet(const std::string& mnemonic,
                     const std::string& password,
                     bool is_legacy_brave_wallet,
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
  void ImportFilecoinSECP256K1Account(
      const std::string& account_name,
      const std::string& private_key,
      const std::string& network,
      ImportFilecoinSECP256K1AccountCallback callback) override;
  void ImportFilecoinBLSAccount(
      const std::string& account_name,
      const std::string& private_key,
      const std::string& public_key,
      const std::string& network,
      ImportFilecoinBLSAccountCallback callback) override;
  void AddHardwareAccounts(
      std::vector<mojom::HardwareWalletAccountPtr> info) override;
  void RemoveHardwareAccount(const std::string& address) override;
  void GetPrivateKeyForImportedAccount(
      const std::string& address,
      GetPrivateKeyForImportedAccountCallback callback) override;
  void RemoveImportedAccount(const std::string& address,
                             RemoveImportedAccountCallback callback) override;
  void IsWalletBackedUp(IsWalletBackedUpCallback callback) override;
  void NotifyWalletBackupComplete() override;
  void GetKeyringsInfo(const std::vector<std::string>& keyrings,
                       GetKeyringsInfoCallback callback) override;
  void GetKeyringInfo(const std::string& keyring_id,
                      GetKeyringInfoCallback callback) override;
  void SetDefaultKeyringHardwareAccountName(
      const std::string& address,
      const std::string& name,
      SetDefaultKeyringHardwareAccountNameCallback callback) override;
  void SetKeyringDerivedAccountName(
      const std::string& keyring_id,
      const std::string& address,
      const std::string& name,
      SetKeyringDerivedAccountNameCallback callback) override;
  void SetKeyringImportedAccountName(
      const std::string& keyring_id,
      const std::string& address,
      const std::string& name,
      SetKeyringImportedAccountNameCallback callback) override;

  void Reset(bool notify_observer = true);
  bool IsKeyringCreated(const std::string& keyring_id);
  bool IsHardwareAccount(const std::string& account) const;
  void SignTransactionByDefaultKeyring(const std::string& address,
                                       EthTransaction* tx,
                                       uint256_t chain_id);

  struct SignatureWithError {
    SignatureWithError();
    SignatureWithError(SignatureWithError&& other);
    SignatureWithError& operator=(SignatureWithError&& other);
    SignatureWithError(const SignatureWithError&) = delete;
    SignatureWithError& operator=(const SignatureWithError&) = delete;
    ~SignatureWithError();

    absl::optional<std::vector<uint8_t>> signature;
    std::string error_message;
  };
  SignatureWithError SignMessageByDefaultKeyring(
      const std::string& address,
      const std::vector<uint8_t>& message,
      bool is_eip712 = false);
  bool RecoverAddressByDefaultKeyring(const std::vector<uint8_t>& message,
                                      const std::vector<uint8_t>& signature,
                                      std::string* address);

  void AddAccountsWithDefaultName(size_t number);

  bool IsLocked() const;
  bool HasPendingUnlockRequest() const;
  void RequestUnlock();
  absl::optional<std::string> GetSelectedAccount() const;

  void AddObserver(
      ::mojo::PendingRemote<mojom::KeyringServiceObserver> observer) override;
  void NotifyUserInteraction() override;
  void GetSelectedAccount(GetSelectedAccountCallback callback) override;
  void SetSelectedAccount(const std::string& address,
                          SetSelectedAccountCallback callback) override;
  void GetAutoLockMinutes(GetAutoLockMinutesCallback callback) override;
  void SetAutoLockMinutes(int32_t minutes,
                          SetAutoLockMinutesCallback callback) override;
  void IsStrongPassword(const std::string& password,
                        IsStrongPasswordCallback callback) override;
  void GetChecksumEthAddress(const std::string& address,
                             GetChecksumEthAddressCallback callback) override;
  void HasPendingUnlockRequest(
      HasPendingUnlockRequestCallback callback) override;

  /* TODO(darkdh): For other keyrings support
  void DeleteKeyring(size_t index);
  HDKeyring* GetKeyring(size_t index);
  */

 private:
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, GetPrefInBytesForKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, SetPrefInBytesForKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, GetOrCreateNonceForKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, CreateEncryptorForKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, CreateDefaultKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest,
                           CreateDefaultKeyringInternal);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, RestoreDefaultKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, UnlockResumesDefaultKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest,
                           GetMnemonicForDefaultKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, LockAndUnlock);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, Reset);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, AccountMetasForKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, CreateAndRestoreWallet);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, AddAccount);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, ImportedAccounts);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest,
                           GetPrivateKeyForDefaultKeyringAccount);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest,
                           SetDefaultKeyringDerivedAccountMeta);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, RestoreLegacyBraveWallet);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, AutoLock);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, SetSelectedAccount);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, UnknownKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, ImportedFilecoinAccounts);
  friend class BraveWalletProviderImplUnitTest;
  friend class EthTxServiceUnitTest;

  void AddAccountForDefaultKeyring(const std::string& account_name);
  mojom::KeyringInfoPtr GetKeyringInfoSync(const std::string& keyring_id);
  void OnAutoLockFired();
  HDKeyring* GetKeyringForAddress(const std::string& address);
  HDKeyring* GetHDKeyringById(const std::string& keyring_id) const;
  std::vector<mojom::AccountInfoPtr> GetHardwareAccountsSync() const;
  std::vector<uint8_t> GetPrivateKeyFromKeyring(const std::string& address,
                                                const std::string& keyring_id);
  // Address will be returned when success
  absl::optional<std::string> ImportAccountForDefaultKeyring(
      const std::string& account_name,
      const std::vector<uint8_t>& private_key);
  absl::optional<std::string> ImportSECP256K1AccountForFilecoinKeyring(
      const std::string& account_name,
      const std::vector<uint8_t>& private_key,
      const std::string& network);
  absl::optional<std::string> ImportBLSAccountForFilecoinKeyring(
      const std::string& account_name,
      const std::vector<uint8_t>& private_key,
      const std::vector<uint8_t>& public_key,
      const std::string& network);
  bool IsFilecoinAccount(const std::string& account) const;
  size_t GetAccountMetasNumberForKeyring(const std::string& id);

  std::vector<mojom::AccountInfoPtr> GetAccountInfosForKeyring(
      const std::string& id);
  bool UpdateNameForHardwareAccountSync(const std::string& address,
                                        const std::string& name);
  const std::string GetMnemonicForKeyringImpl(const std::string& keyring_id);

  bool GetPrefInBytesForKeyring(const std::string& key,
                                std::vector<uint8_t>* bytes,
                                const std::string& id) const;
  void SetPrefInBytesForKeyring(const std::string& key,
                                base::span<const uint8_t> bytes,
                                const std::string& id);
  std::vector<uint8_t> GetOrCreateNonceForKeyring(const std::string& id);
  bool CreateEncryptorForKeyring(const std::string& password,
                                 const std::string& id);
  bool CreateKeyringInternal(const std::string& keyring_id,
                             const std::string& mnemonic,
                             bool is_legacy_brave_wallet);

  // Currently only support one default keyring, `CreateDefaultKeyring` and
  // `RestoreDefaultKeyring` will overwrite existing one if success
  HDKeyring* CreateKeyring(const std::string& keyring_id,
                           const std::string& password);
  // Restore default keyring from backup seed phrase
  HDKeyring* RestoreKeyring(const std::string& keyring_id,
                            const std::string& mnemonic,
                            const std::string& password,
                            bool is_legacy_brave_wallet);
  // It's used to reconstruct same default keyring between browser relaunch
  HDKeyring* ResumeKeyring(const std::string& keyring_id,
                           const std::string& password);

  void NotifyAccountsChanged();
  void StopAutoLockTimer();
  void ResetAutoLockTimer();
  void OnAutoLockPreferenceChanged();
  void OnSelectedAccountPreferenceChanged();

  std::unique_ptr<PasswordEncryptor> encryptor_;
  std::unique_ptr<base::OneShotTimer> auto_lock_timer_;
  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;
  base::flat_map<std::string, std::unique_ptr<HDKeyring>> keyrings_;

  raw_ptr<PrefService> prefs_ = nullptr;
  bool request_unlock_pending_ = false;

  mojo::RemoteSet<mojom::KeyringServiceObserver> observers_;
  mojo::ReceiverSet<mojom::KeyringService> receivers_;

  KeyringService(const KeyringService&) = delete;
  KeyringService& operator=(const KeyringService&) = delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_SERVICE_H_
