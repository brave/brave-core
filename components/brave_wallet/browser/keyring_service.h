/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_SERVICE_H_

#include <array>
#include <memory>
#include <optional>
#include <string>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_hd_keyring.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "brave/components/brave_wallet/common/buildflags.h"
#include "brave/components/brave_wallet/common/zcash_utils.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"

class PrefChangeRegistrar;
class PrefService;

namespace base {
class OneShotTimer;
}

namespace brave_wallet {

class AccountDiscoveryManager;
class AssetDiscoveryManagerUnitTest;
class BitcoinHDKeyring;
class BitcoinHardwareKeyring;
class BitcoinImportKeyring;
class EthereumKeyring;
class EthereumProviderImplUnitTest;
class EthTransaction;
class CardanoHDKeyring;
class FilecoinKeyring;
class FilTransaction;
class JsonRpcService;
class KeyringServiceUnitTest;
class PasswordEncryptor;
class SolanaKeyring;
class SolanaProviderImplUnitTest;
class ZCashKeyring;
struct KeyringSeed;

// This class is not thread-safe and should have single owner
class KeyringService : public mojom::KeyringService {
 public:
  KeyringService(JsonRpcService* json_rpc_service,
                 PrefService* profile_prefs,
                 PrefService* local_state);
  ~KeyringService() override;

  void Bind(mojo::PendingReceiver<mojom::KeyringService> receiver);

  // mojom::KeyringService
  // Must unlock before using this API otherwise it will return empty string
  void GetWalletMnemonic(const std::string& password,
                         GetWalletMnemonicCallback callback) override;
  void IsWalletCreated(IsWalletCreatedCallback callback) override;
  bool IsWalletCreatedSync();
  void CreateWallet(const std::string& password,
                    CreateWalletCallback callback) override;
  void RestoreWallet(const std::string& mnemonic,
                     const std::string& password,
                     bool is_legacy_eth_seed_format,
                     RestoreWalletCallback callback) override;
  bool RestoreWalletSync(const std::string& mnemonic,
                         const std::string& password,
                         bool is_legacy_eth_seed_format);
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
  void ImportEthereumAccount(const std::string& account_name,
                             const std::string& private_key,
                             ImportEthereumAccountCallback callback) override;
  void ImportSolanaAccount(const std::string& account_name,
                           const std::string& private_key,
                           ImportSolanaAccountCallback callback) override;
  void ImportEthereumAccountFromJson(
      const std::string& account_name,
      const std::string& password,
      const std::string& json,
      ImportEthereumAccountFromJsonCallback callback) override;
  void ImportFilecoinAccount(const std::string& account_name,
                             const std::string& private_key,
                             const std::string& network,
                             ImportFilecoinAccountCallback callback) override;
  void ImportBitcoinAccount(const std::string& account_name,
                            const std::string& payload,
                            const std::string& network,
                            ImportBitcoinAccountCallback callback) override;
  mojom::AccountInfoPtr ImportBitcoinAccountSync(
      const std::string& account_name,
      const std::string& payload,
      const std::string& network);
  void AddHardwareAccounts(std::vector<mojom::HardwareWalletAccountPtr> info,
                           AddHardwareAccountsCallback callback) override;
  std::vector<mojom::AccountInfoPtr> AddHardwareAccountsSync(
      std::vector<mojom::HardwareWalletAccountPtr> info);
  void AddBitcoinHardwareAccount(
      mojom::HardwareWalletAccountPtr info,
      AddBitcoinHardwareAccountCallback callback) override;
  mojom::AccountInfoPtr AddBitcoinHardwareAccountSync(
      mojom::HardwareWalletAccountPtr info);
  void RemoveAccount(mojom::AccountIdPtr account_id,
                     const std::string& password,
                     RemoveAccountCallback callback) override;
  void IsWalletBackedUp(IsWalletBackedUpCallback callback) override;
  bool IsWalletBackedUpSync();
  void NotifyWalletBackupComplete() override;
  void SetAccountName(mojom::AccountIdPtr account_id,
                      const std::string& name,
                      SetAccountNameCallback callback) override;
  void Reset(bool notify_observer = true);
  void SignTransactionByDefaultKeyring(const mojom::AccountIdPtr& account_id,
                                       EthTransaction* tx,
                                       uint256_t chain_id);
  std::optional<std::string> SignTransactionByFilecoinKeyring(
      const mojom::AccountIdPtr& account_id,
      const FilTransaction& tx);
  std::optional<std::string> GetDiscoveryAddress(mojom::KeyringId keyring_id,
                                                 int index);

  base::expected<std::vector<uint8_t>, std::string> SignMessageByDefaultKeyring(
      const mojom::AccountIdPtr& account_id,
      base::span<const uint8_t> message,
      bool is_eip712);

  std::vector<uint8_t> SignMessageBySolanaKeyring(
      const mojom::AccountIdPtr& account_id,
      base::span<const uint8_t> message);
  std::optional<std::string> RecoverAddressByDefaultKeyring(
      base::span<const uint8_t> message,
      base::span<const uint8_t> eth_signature);
  bool GetPublicKeyFromX25519_XSalsa20_Poly1305ByDefaultKeyring(
      const mojom::AccountIdPtr& account_id,
      std::string* key);
  std::optional<std::vector<uint8_t>>
  DecryptCipherFromX25519_XSalsa20_Poly1305ByDefaultKeyring(
      const mojom::AccountIdPtr& account_id,
      const std::string& version,
      base::span<const uint8_t> nonce,
      base::span<const uint8_t> ephemeral_public_key,
      base::span<const uint8_t> ciphertext);

  void AddAccountsWithDefaultName(const mojom::CoinType& coin_type,
                                  mojom::KeyringId keyring_id,
                                  size_t number);

  bool IsLockedSync();
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

  void UpdateNextUnusedAddressForBitcoinAccount(
      const mojom::AccountIdPtr& account_id,
      std::optional<uint32_t> next_receive_index,
      std::optional<uint32_t> next_change_index);
  mojom::BitcoinAccountInfoPtr GetBitcoinAccountInfo(
      const mojom::AccountIdPtr& account_id);
  std::optional<std::vector<mojom::BitcoinAddressPtr>> GetBitcoinAddresses(
      const mojom::AccountIdPtr& account_id);
  mojom::BitcoinAddressPtr GetBitcoinAddress(
      const mojom::AccountIdPtr& account_id,
      const mojom::BitcoinKeyIdPtr& key_id);
  mojom::BitcoinAddressPtr GetBitcoinAccountDiscoveryAddress(
      const mojom::KeyringId keyring_id,
      uint32_t account_index,
      const mojom::BitcoinKeyIdPtr& key_id);
  std::optional<std::vector<uint8_t>> GetBitcoinPubkey(
      const mojom::AccountIdPtr& account_id,
      const mojom::BitcoinKeyIdPtr& key_id);
  std::optional<std::vector<uint8_t>> SignMessageByBitcoinKeyring(
      const mojom::AccountIdPtr& account_id,
      const mojom::BitcoinKeyIdPtr& key_id,
      base::span<const uint8_t, 32> message);

  /* ZCash */
  // ZCash account birthday represents block in the blockchain where
  // account was used at first time. It is used as a start index when
  // zcash blockchain is being scanned.
  bool SetZCashAccountBirthday(
      const mojom::AccountIdPtr& account_id,
      mojom::ZCashAccountShieldBirthdayPtr account_birthday);
  void UpdateNextUnusedAddressForZCashAccount(
      const mojom::AccountIdPtr& account_id,
      std::optional<uint32_t> next_receive_index,
      std::optional<uint32_t> next_change_index);
  mojom::ZCashAccountInfoPtr GetZCashAccountInfo(
      const mojom::AccountIdPtr& account_id);
  std::optional<std::vector<uint8_t>> SignMessageByZCashKeyring(
      const mojom::AccountIdPtr& account_id,
      const mojom::ZCashKeyIdPtr& key_id,
      const base::span<const uint8_t, 32> message);
  mojom::ZCashAddressPtr GetZCashAddress(const mojom::AccountIdPtr& account_id,
                                         const mojom::ZCashKeyId& key_id);
  std::optional<std::vector<mojom::ZCashAddressPtr>> GetZCashAddresses(
      const mojom::AccountIdPtr& account_id);
  std::optional<std::vector<uint8_t>> GetZCashPubKey(
      const mojom::AccountIdPtr& account_id,
      const mojom::ZCashKeyIdPtr& key_id);
#if BUILDFLAG(ENABLE_ORCHARD)
  std::optional<OrchardAddrRawPart> GetOrchardRawBytes(
      const mojom::AccountIdPtr& account_id,
      const mojom::ZCashKeyIdPtr& key_id);
  std::optional<OrchardFullViewKey> GetOrchardFullViewKey(
      const mojom::AccountIdPtr& account_id);
  std::optional<OrchardSpendingKey> GetOrchardSpendingKey(
      const mojom::AccountIdPtr& account_id);
#endif

  void UpdateNextUnusedAddressForCardanoAccount(
      const mojom::AccountIdPtr& account_id,
      std::optional<uint32_t> next_external_index,
      std::optional<uint32_t> next_internal_index);
  mojom::CardanoAccountInfoPtr GetCardanoAccountInfo(
      const mojom::AccountIdPtr& account_id);
  std::optional<std::vector<mojom::CardanoAddressPtr>> GetCardanoAddresses(
      const mojom::AccountIdPtr& account_id);
  mojom::CardanoAddressPtr GetCardanoAddress(
      const mojom::AccountIdPtr& account_id,
      const mojom::CardanoKeyIdPtr& payment_key_id);
  std::optional<CardanoSignMessageResult> SignMessageByCardanoKeyring(
      const mojom::AccountIdPtr& account_id,
      const mojom::CardanoKeyIdPtr& key_id,
      base::span<const uint8_t> message);
  std::optional<base::Value::Dict> SignCip30MessageByCardanoKeyring(
      const mojom::AccountIdPtr& account_id,
      const mojom::CardanoKeyIdPtr& key_id,
      base::span<const uint8_t> message);

  const std::vector<mojom::AccountInfoPtr>& GetAllAccountInfos();
  mojom::AccountInfoPtr FindAccount(const mojom::AccountIdPtr& account_id);
  mojom::AccountInfoPtr GetSelectedWalletAccount();
  mojom::AccountInfoPtr GetSelectedEthereumDappAccount();
  mojom::AccountInfoPtr GetSelectedSolanaDappAccount();
  mojom::AccountInfoPtr GetSelectedCardanoDappAccount();
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
  FRIEND_TEST_ALL_PREFIXES(KeyringServiceUnitTest, GetWalletMnemonic);
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

  friend class AccountUtils;
  friend class BraveWalletServiceUnitTest;
  friend class EthereumProviderImplUnitTest;
  friend class SolanaProviderImplUnitTest;
  friend class KeyringServiceAccountDiscoveryUnitTest;
  friend class EthTxManagerUnitTest;
  friend class FilTxManagerUnitTest;
  friend class KeyringServiceUnitTest;
  friend class AssetDiscoveryManagerUnitTest;
  friend class SolanaTransactionUnitTest;

  void ResetAllAccountInfosCache();
  mojom::AccountInfoPtr AddHDAccountForKeyring(mojom::KeyringId keyring_id,
                                               const std::string& account_name);
  std::optional<std::string> AddHDAccountForKeyringInternal(
      mojom::KeyringId keyring_id,
      uint32_t index);
  bool RemoveImportedAccountInternal(const mojom::AccountIdPtr& account_id);
  bool RemoveHardwareAccountInternal(const mojom::AccountId& account_id);
  bool SetKeyringDerivedAccountNameInternal(const mojom::AccountId& account_id,
                                            const std::string& name);
  bool SetKeyringImportedAccountNameInternal(const mojom::AccountId& account_id,
                                             const std::string& name);
  bool SetHardwareAccountNameInternal(const mojom::AccountId& account_id,
                                      const std::string& name);

  void OnAutoLockFired();

  template <typename T>
  T* GetKeyring(mojom::KeyringId keyring_id) const;
  template <typename T>
  T* GetKeyring(const mojom::AccountIdPtr& account_id) const;

  std::vector<mojom::AccountInfoPtr> GetHardwareAccountsSync(
      mojom::KeyringId keyring_id) const;
  mojom::AccountInfoPtr ImportAccountForKeyring(
      mojom::KeyringId keyring_id,
      const std::string& account_name,
      base::span<const uint8_t> private_key);

  std::vector<mojom::AccountInfoPtr> GetAccountInfosForKeyring(
      mojom::KeyringId keyring_id) const;

  bool CanResumeWallet(const std::string& mnemonic,
                       const std::string& password,
                       bool is_legacy_eth_seed_format);
  bool CreateWalletInternal(const std::string& mnemonic,
                            const std::string& password,
                            bool is_legacy_eth_seed_format,
                            bool from_restore);
  void CreateKeyringInternal(mojom::KeyringId keyring_id,
                             const KeyringSeed& keyring_seed);
  bool IsKeyringEnabled(mojom::KeyringId keyring_id) const;
  void CreateKeyrings(const KeyringSeed& keyring_seed);
  void ClearKeyrings();
  void CreateDefaultAccounts();
  void LoadAllAccountsFromPrefs();
  void LoadAccountsFromPrefs(mojom::KeyringId keyring_id);

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
  std::optional<std::string> GetWalletMnemonicInternal(
      const std::string& password);
  bool CreateEncryptorAndValidatePasswordInternal(const std::string& password);
  void MaybeUnlockWithCommandLine();

  std::unique_ptr<std::vector<mojom::AccountInfoPtr>> account_info_cache_;
  std::unique_ptr<base::OneShotTimer> auto_lock_timer_;
  std::unique_ptr<PrefChangeRegistrar> pref_change_registrar_;

  std::vector<mojom::KeyringId> enabled_keyrings_;

  std::unique_ptr<EthereumKeyring> ethereum_keyring_;

  std::unique_ptr<SolanaKeyring> solana_keyring_;

  std::unique_ptr<FilecoinKeyring> filecoin_mainnet_keyring_;
  std::unique_ptr<FilecoinKeyring> filecoin_testnet_keyring_;

  std::unique_ptr<BitcoinHDKeyring> bitcoin_hd_mainnet_keyring_;
  std::unique_ptr<BitcoinHDKeyring> bitcoin_hd_testnet_keyring_;
  std::unique_ptr<BitcoinImportKeyring> bitcoin_import_mainnet_keyring_;
  std::unique_ptr<BitcoinImportKeyring> bitcoin_import_testnet_keyring_;
  std::unique_ptr<BitcoinHardwareKeyring> bitcoin_hardware_mainnet_keyring_;
  std::unique_ptr<BitcoinHardwareKeyring> bitcoin_hardware_testnet_keyring_;

  std::unique_ptr<ZCashKeyring> zcash_hd_mainnet_keyring_;
  std::unique_ptr<ZCashKeyring> zcash_hd_testnet_keyring_;

  std::unique_ptr<CardanoHDKeyring> cardano_hd_mainnet_keyring_;
  std::unique_ptr<CardanoHDKeyring> cardano_hd_testnet_keyring_;

  std::unique_ptr<PasswordEncryptor> encryptor_;

  raw_ptr<JsonRpcService> json_rpc_service_;
  raw_ptr<PrefService> profile_prefs_ = nullptr;
  raw_ptr<PrefService> local_state_ = nullptr;
  bool request_unlock_pending_ = false;

  mojo::RemoteSet<mojom::KeyringServiceObserver> observers_;
  mojo::ReceiverSet<mojom::KeyringService> receivers_;

  KeyringService(const KeyringService&) = delete;
  KeyringService& operator=(const KeyringService&) = delete;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_KEYRING_SERVICE_H_
