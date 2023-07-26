/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_SERVICE_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/values.h"
#include "brave/components/brave_wallet/browser/account_discovery_manager.h"
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

class BitcoinKeyring;
class AccountDiscoveryManager;
class EthTransaction;
class FilTransaction;
class KeyringServiceUnitTest;
class EthereumProviderImplUnitTest;
class SolanaProviderImplUnitTest;
class FilecoinKeyring;
class JsonRpcService;
class AssetDiscoveryManagerUnitTest;

// This class is not thread-safe and should have single owner
class KeyringService : public KeyedService, public mojom::KeyringService {
 public:
  KeyringService(JsonRpcService* json_rpc_service,
                 PrefService* profile_prefs,
                 PrefService* local_state);
  ~KeyringService() override;

  static void MigrateDerivedAccountIndex(PrefService* profile_prefs);

  void MaybeMigrateSelectedAccountPrefs();
  static absl::optional<int>& GetPbkdf2IterationsForTesting();
  static void MigrateObsoleteProfilePrefs(PrefService* profile_prefs);

  static bool HasPrefForKeyring(const PrefService& profile_prefs,
                                const std::string& key,
                                mojom::KeyringId keyring_id);
  static const base::Value* GetPrefForKeyring(const PrefService& profile_prefs,
                                              const std::string& key,
                                              mojom::KeyringId keyring_id);
  // For testing only.
  static void SetPrefForKeyring(PrefService* profile_prefs,
                                const std::string& key,
                                base::Value value,
                                mojom::KeyringId keyring_id);
  static absl::optional<std::vector<uint8_t>> GetPrefInBytesForKeyring(
      const PrefService& profile_prefs,
      const std::string& key,
      mojom::KeyringId keyring_id);
  static void SetPrefInBytesForKeyring(PrefService* profile_prefs,
                                       const std::string& key,
                                       base::span<const uint8_t> bytes,
                                       mojom::KeyringId keyring_id);

  static absl::optional<mojom::KeyringId> GetKeyringIdForCoinNonFIL(
      mojom::CoinType coin);

  mojo::PendingRemote<mojom::KeyringService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::KeyringService> receiver);

  // mojom::KeyringService
  // Must unlock before using this API otherwise it will return empty string
  void GetMnemonicForDefaultKeyring(
      const std::string& password,
      GetMnemonicForDefaultKeyringCallback callback) override;
  void MaybeCreateDefaultSolanaAccount();
  void CreateWallet(const std::string& password,
                    CreateWalletCallback callback) override;
  void RestoreWallet(const std::string& mnemonic,
                     const std::string& password,
                     bool is_legacy_brave_wallet,
                     RestoreWalletCallback callback) override;
  void Unlock(const std::string& password, UnlockCallback callback) override;
  void Lock() override;
  void IsLocked(IsLockedCallback callback) override;
  void AddAccount(mojom::CoinType coin,
                  mojom::KeyringId keyring_id,
                  const std::string& account_name,
                  AddAccountCallback callback) override;
  mojom::AccountInfoPtr AddAccountSync(mojom::CoinType coin,
                                       mojom::KeyringId keyring_id,
                                       const std::string& account_name);
  void EncodePrivateKeyForExport(
      mojom::AccountIdPtr account_id,
      const std::string& password,
      EncodePrivateKeyForExportCallback callback) override;
  void ImportAccount(const std::string& account_name,
                     const std::string& private_key,
                     mojom::CoinType coin,
                     ImportAccountCallback callback) override;
  void ImportAccountFromJson(const std::string& account_name,
                             const std::string& password,
                             const std::string& json,
                             ImportAccountCallback callback) override;
  void ImportFilecoinAccount(const std::string& account_name,
                             const std::string& private_key,
                             const std::string& network,
                             ImportFilecoinAccountCallback callback) override;
  void AddHardwareAccounts(std::vector<mojom::HardwareWalletAccountPtr> info,
                           AddHardwareAccountsCallback callback) override;
  std::vector<mojom::AccountInfoPtr> AddHardwareAccountsSync(
      std::vector<mojom::HardwareWalletAccountPtr> info);
  void RemoveAccount(mojom::AccountIdPtr account_id,
                     const std::string& password,
                     RemoveAccountCallback callback) override;
  void IsWalletBackedUp(IsWalletBackedUpCallback callback) override;
  void NotifyWalletBackupComplete() override;
  mojom::KeyringInfoPtr GetKeyringInfoSync(mojom::KeyringId keyring_id);
  void GetKeyringInfo(mojom::KeyringId keyring_id,
                      GetKeyringInfoCallback callback) override;
  void SetAccountName(mojom::AccountIdPtr account_id,
                      const std::string& name,
                      SetAccountNameCallback callback) override;
  void Reset(bool notify_observer = true);
  bool IsKeyringCreated(mojom::KeyringId keyring_id) const;
  bool IsHardwareAccount(mojom::KeyringId keyring_id,
                         const std::string& account) const;
  void SignTransactionByDefaultKeyring(const std::string& address,
                                       EthTransaction* tx,
                                       uint256_t chain_id);
  absl::optional<std::string> SignTransactionByFilecoinKeyring(
      FilTransaction* tx);
  absl::optional<std::string> GetDiscoveryAddress(mojom::KeyringId keyring_id,
                                                  int index);

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

  std::vector<uint8_t> SignMessageBySolanaKeyring(
      const mojom::AccountId& account_id,
      const std::vector<uint8_t>& message);
  bool RecoverAddressByDefaultKeyring(const std::vector<uint8_t>& message,
                                      const std::vector<uint8_t>& signature,
                                      std::string* address);
  bool GetPublicKeyFromX25519_XSalsa20_Poly1305ByDefaultKeyring(
      const std::string& address,
      std::string* key);
  absl::optional<std::vector<uint8_t>>
  DecryptCipherFromX25519_XSalsa20_Poly1305ByDefaultKeyring(
      const std::string& version,
      const std::vector<uint8_t>& nonce,
      const std::vector<uint8_t>& ephemeral_public_key,
      const std::vector<uint8_t>& ciphertext,
      const std::string& address);

  void AddAccountsWithDefaultName(const mojom::CoinType& coin_type,
                                  mojom::KeyringId keyring_id,
                                  size_t number);

  bool IsLocked(mojom::KeyringId keyring_id) const;
  bool IsLockedSync() const;
  bool HasPendingUnlockRequest() const;
  void RequestUnlock();

  void AddObserver(
      ::mojo::PendingRemote<mojom::KeyringServiceObserver> observer) override;
  void NotifyUserInteraction() override;
  void GetAllAccounts(GetAllAccountsCallback callback) override;
  mojom::AllAccountsInfoPtr GetAllAccountsSync();
  void SetSelectedAccount(mojom::AccountIdPtr account_id,
                          SetSelectedAccountCallback callback) override;
  bool SetSelectedAccountSync(mojom::AccountIdPtr account_id);
  void GetAutoLockMinutes(GetAutoLockMinutesCallback callback) override;
  void SetAutoLockMinutes(int32_t minutes,
                          SetAutoLockMinutesCallback callback) override;
  void IsStrongPassword(const std::string& password,
                        IsStrongPasswordCallback callback) override;
  void ValidatePassword(const std::string& password,
                        ValidatePasswordCallback callback) override;
  void GetChecksumEthAddress(const std::string& address,
                             GetChecksumEthAddressCallback callback) override;
  void HasPendingUnlockRequest(
      HasPendingUnlockRequestCallback callback) override;
  absl::optional<size_t> GetAccountsNumber(mojom::KeyringId keyring_id);

  absl::optional<std::vector<std::pair<std::string, mojom::BitcoinKeyIdPtr>>>
  GetBitcoinAddresses(const mojom::AccountId& account_id);
  absl::optional<std::string> GetBitcoinAddress(
      const mojom::AccountId& account_id,
      const mojom::BitcoinKeyId& key_id);
  absl::optional<std::vector<uint8_t>> GetBitcoinPubkey(
      const mojom::AccountId& account_id,
      const mojom::BitcoinKeyId& key_id);
  absl::optional<std::vector<uint8_t>> SignMessageByBitcoinKeyring(
      const mojom::AccountId& account_id,
      const mojom::BitcoinKeyId& key_id,
      base::span<const uint8_t, 32> message);

  std::vector<mojom::AccountInfoPtr> GetAllAccountInfos();
  mojom::AccountInfoPtr GetSelectedWalletAccount();
  mojom::AccountInfoPtr GetSelectedEthereumDappAccount();
  mojom::AccountInfoPtr GetSelectedSolanaDappAccount();
  void MaybeFixAccountSelection();

 private:
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, GetOrCreateNonceForKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, GetOrCreateSaltForKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, CreateEncryptorForKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, CreateDefaultKeyring);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest,
                           LazyCreateFilecoinKeyringFromImport);
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
                           SetDefaultKeyringDerivedAccountMeta);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, RestoreLegacyBraveWallet);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, AutoLock);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, SetSelectedAccount);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, ImportFilecoinAccounts);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, PreCreateEncryptors);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, HardwareAccounts);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, DefaultSolanaAccountCreated);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest,
                           DefaultSolanaAccountRestored);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, AccountsAdded);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceAccountDiscoveryUnitTest,
                           AccountDiscovery);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceAccountDiscoveryUnitTest,
                           SolAccountDiscovery);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceAccountDiscoveryUnitTest,
                           FilAccountDiscovery);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceAccountDiscoveryUnitTest,
                           StopsOnError);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceAccountDiscoveryUnitTest,
                           ManuallyAddAccount);
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceAccountDiscoveryUnitTest,
                           RestoreWalletTwice);
  FRIEND_TEST_ALL_PREFIXES(AssetDiscoveryManagerUnitTest,
                           KeyringServiceObserver);
  FRIEND_TEST_ALL_PREFIXES(SolanaProviderImplUnitTest,
                           ConnectWithNoSolanaAccount);

  friend class BraveWalletServiceUnitTest;
  friend class EthereumProviderImplUnitTest;
  friend class SolanaProviderImplUnitTest;
  friend class KeyringServiceAccountDiscoveryUnitTest;
  friend class EthTxManagerUnitTest;
  friend class FilTxManagerUnitTest;
  friend class KeyringServiceUnitTest;
  friend class AssetDiscoveryManagerUnitTest;
  friend class SolanaTransactionUnitTest;

  mojom::AccountInfoPtr AddAccountForKeyring(mojom::KeyringId keyring_id,
                                             const std::string& account_name);
  bool RemoveImportedAccountInternal(const mojom::AccountId& account_id);
  bool RemoveHardwareAccountInternal(const mojom::AccountId& account_id);
  bool SetKeyringDerivedAccountNameInternal(const mojom::AccountId& account_id,
                                            const std::string& name);
  bool SetKeyringImportedAccountNameInternal(const mojom::AccountId& account_id,
                                             const std::string& name);
  bool SetHardwareAccountNameInternal(const mojom::AccountId& account_id,
                                      const std::string& name);

  void OnAutoLockFired();
  HDKeyring* GetHDKeyringById(mojom::KeyringId keyring_id) const;
  BitcoinKeyring* GetBitcoinKeyringById(mojom::KeyringId keyring_id) const;
  std::vector<mojom::AccountInfoPtr> GetHardwareAccountsSync(
      mojom::KeyringId keyring_id) const;
  // Address will be returned when success
  mojom::AccountInfoPtr ImportAccountForKeyring(
      mojom::CoinType coin,
      mojom::KeyringId keyring_id,
      const std::string& account_name,
      const std::vector<uint8_t>& private_key);
  bool IsKeyringExist(mojom::KeyringId keyring_id) const;
  bool LazilyCreateKeyring(mojom::KeyringId keyring_id);

  std::vector<mojom::AccountInfoPtr> GetAccountInfosForKeyring(
      mojom::KeyringId keyring_id) const;
  std::string GetMnemonicForKeyringImpl(mojom::KeyringId keyring_id);

  std::vector<uint8_t> GetOrCreateNonceForKeyring(mojom::KeyringId keyring_id,
                                                  bool force_create = false);
  std::vector<uint8_t> GetOrCreateSaltForKeyring(mojom::KeyringId keyring_id,
                                                 bool force_create = false);
  bool CreateEncryptorForKeyring(const std::string& password,
                                 mojom::KeyringId keyring_id);
  HDKeyring* CreateKeyringInternal(mojom::KeyringId keyring_id,
                                   const std::string& mnemonic,
                                   bool is_legacy_brave_wallet);

  // Currently only support one default keyring, `CreateDefaultKeyring` and
  // `RestoreDefaultKeyring` will overwrite existing one if success
  HDKeyring* CreateKeyring(mojom::KeyringId keyring_id,
                           const std::string& mnemonic,
                           const std::string& password);
  // Restore default keyring from backup seed phrase
  HDKeyring* RestoreKeyring(mojom::KeyringId keyring_id,
                            const std::string& mnemonic,
                            const std::string& password,
                            bool is_legacy_brave_wallet);
  // It's used to reconstruct same default keyring between browser relaunch
  HDKeyring* ResumeKeyring(mojom::KeyringId keyring_id,
                           const std::string& password);

  void MaybeMigratePBKDF2Iterations(const std::string& password);

  void NotifyAccountsChanged();
  void NotifyAccountsAdded(const mojom::AccountInfo& added_account);
  void NotifyAccountsAdded(
      const std::vector<mojom::AccountInfoPtr>& added_accounts);
  void StopAutoLockTimer();
  void ResetAutoLockTimer();
  void OnAutoLockPreferenceChanged();
  mojom::AccountInfoPtr GetSelectedDappAccount(mojom::CoinType coin);
  void NotifySelectedWalletAccountChanged(const mojom::AccountInfo& account);
  void NotifySelectedDappAccountChanged(mojom::CoinType coin,
                                        const mojom::AccountInfoPtr& account);
  void SetSelectedAccountInternal(const mojom::AccountInfo& account_info);
  void SetSelectedWalletAccountInternal(const mojom::AccountInfo& account_info);
  void SetSelectedDappAccountInternal(
      mojom::CoinType coin,
      const mojom::AccountInfoPtr& account_info);
  void AddHardwareAccounts(std::vector<mojom::HardwareWalletAccountPtr> info,
                           mojom::KeyringId keyring_id);

  bool ValidatePasswordInternal(const std::string& password);
  void MaybeUnlockWithCommandLine();

  std::unique_ptr<base::OneShotTimer> auto_lock_timer_;
  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;
  base::flat_map<mojom::KeyringId, std::unique_ptr<HDKeyring>> keyrings_;
  base::flat_map<mojom::KeyringId, std::unique_ptr<PasswordEncryptor>>
      encryptors_;

  raw_ptr<JsonRpcService> json_rpc_service_;
  raw_ptr<PrefService> profile_prefs_ = nullptr;
  raw_ptr<PrefService> local_state_ = nullptr;
  bool request_unlock_pending_ = false;

  mojo::RemoteSet<mojom::KeyringServiceObserver> observers_;
  mojo::ReceiverSet<mojom::KeyringService> receivers_;

  std::unique_ptr<AccountDiscoveryManager> account_discovery_manager_;

  KeyringService(const KeyringService&) = delete;
  KeyringService& operator=(const KeyringService&) = delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_SERVICE_H_
