/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "base/containers/circular_deque.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/components/brave_wallet/browser/brave_wallet_p3a.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/origin.h"

class PrefService;

namespace brave_wallet {

constexpr char kBraveWalletWeeklyHistogramName[] =
    "Brave.Wallet.UsageDaysInWeek";
constexpr char kBraveWalletMonthlyHistogramName[] =
    "Brave.Wallet.UsageMonthly.2";
constexpr char kBraveWalletNewUserReturningHistogramName[] =
    "Brave.Wallet.NewUserReturning";
constexpr char kBraveWalletOnboardingConvHistogramName[] =
    "Brave.Wallet.OnboardingConversion";

class KeyringService;
class JsonRpcService;
class TxService;

class BraveWalletService : public KeyedService,
                           public mojom::BraveWalletService,
                           public BraveWalletServiceDelegate::Observer {
 public:
  using SignMessageRequestCallback =
      base::OnceCallback<void(bool,
                              mojom::ByteArrayStringUnionPtr,
                              const absl::optional<std::string>&)>;
  using SignTransactionRequestCallback =
      base::OnceCallback<void(bool,
                              mojom::ByteArrayStringUnionPtr,
                              const absl::optional<std::string>&)>;
  using SignAllTransactionsRequestCallback = base::OnceCallback<void(
      bool,
      absl::optional<std::vector<mojom::ByteArrayStringUnionPtr>>,
      const absl::optional<std::string>&)>;
  using AddSuggestTokenCallback =
      base::OnceCallback<void(bool, mojom::ProviderError, const std::string&)>;

  BraveWalletService(std::unique_ptr<BraveWalletServiceDelegate> delegate,
                     KeyringService* keyring_service,
                     JsonRpcService* json_rpc_service,
                     TxService* tx_service,
                     PrefService* prefs);
  ~BraveWalletService() override;

  BraveWalletService(const BraveWalletService&) = delete;
  BraveWalletService& operator=(const BraveWalletService&) = delete;

  mojo::PendingRemote<mojom::BraveWalletService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::BraveWalletService> receiver);

  static void MigrateUserAssetEthContractAddress(PrefService* prefs);
  static void MigrateMultichainUserAssets(PrefService* prefs);
  static void MigrateUserAssetsAddPreloadingNetworks(PrefService* prefs);

  static base::Value::Dict GetDefaultEthereumAssets();
  static base::Value::Dict GetDefaultSolanaAssets();
  static base::Value::Dict GetDefaultFilecoinAssets();

  // mojom::BraveWalletService:
  void AddObserver(::mojo::PendingRemote<mojom::BraveWalletServiceObserver>
                       observer) override;

  void GetUserAssets(const std::string& chain_id,
                     mojom::CoinType coin,
                     GetUserAssetsCallback callback) override;
  void AddUserAsset(mojom::BlockchainTokenPtr token,
                    AddUserAssetCallback callback) override;
  void RemoveUserAsset(mojom::BlockchainTokenPtr token,
                       RemoveUserAssetCallback callback) override;
  void SetUserAssetVisible(mojom::BlockchainTokenPtr token,
                           bool visible,
                           SetUserAssetVisibleCallback callback) override;
  void IsExternalWalletInstalled(mojom::ExternalWalletType,
                                 IsExternalWalletInstalledCallback) override;
  void IsExternalWalletInitialized(
      mojom::ExternalWalletType,
      IsExternalWalletInitializedCallback) override;
  void ImportFromExternalWallet(
      mojom::ExternalWalletType type,
      const std::string& password,
      const std::string& new_password,
      ImportFromExternalWalletCallback callback) override;

  void GetDefaultWallet(GetDefaultWalletCallback callback) override;
  void SetDefaultWallet(mojom::DefaultWallet default_wallet) override;
  void GetDefaultBaseCurrency(GetDefaultBaseCurrencyCallback callback) override;
  void SetDefaultBaseCurrency(const std::string& currency) override;
  void GetDefaultBaseCryptocurrency(
      GetDefaultBaseCryptocurrencyCallback callback) override;
  void SetDefaultBaseCryptocurrency(const std::string& cryptocurrency) override;
  void GetShowWalletTestNetworks(
      GetShowWalletTestNetworksCallback callback) override;
  void GetSelectedCoin(GetSelectedCoinCallback callback) override;
  void SetSelectedCoin(mojom::CoinType coin) override;
  void AddPermission(mojom::CoinType coin,
                     const url::Origin& origin,
                     const std::string& account,
                     AddPermissionCallback callback) override;
  void HasPermission(mojom::CoinType coin,
                     const url::Origin& origin,
                     const std::string& account,
                     HasPermissionCallback callback) override;
  void ResetPermission(mojom::CoinType coin,
                       const url::Origin& origin,
                       const std::string& account,
                       ResetPermissionCallback callback) override;
  void IsPermissionDenied(mojom::CoinType coin,
                          const url::Origin& origin,
                          IsPermissionDeniedCallback callback) override;
  void GetWebSitesWithPermission(
      mojom::CoinType coin,
      GetWebSitesWithPermissionCallback callback) override;
  void ResetWebSitePermission(mojom::CoinType coin,
                              const std::string& formed_website,
                              ResetWebSitePermissionCallback callback) override;
  void GetActiveOrigin(GetActiveOriginCallback callback) override;
  void GeteTLDPlusOneFromOrigin(const url::Origin& origin,
                                GetActiveOriginCallback callback) override;
  void GetPendingSignMessageRequests(
      GetPendingSignMessageRequestsCallback callback) override;
  void GetPendingSignTransactionRequests(
      GetPendingSignTransactionRequestsCallback callback) override;
  void GetPendingSignAllTransactionsRequests(
      GetPendingSignAllTransactionsRequestsCallback callback) override;
  void NotifySignTransactionRequestProcessed(
      bool approved,
      int id,
      mojom::ByteArrayStringUnionPtr signature,
      const absl::optional<std::string>& error) override;
  void NotifySignAllTransactionsRequestProcessed(
      bool approved,
      int id,
      absl::optional<std::vector<mojom::ByteArrayStringUnionPtr>> signatures,
      const absl::optional<std::string>& error) override;
  void NotifySignMessageRequestProcessed(
      bool approved,
      int id,
      mojom::ByteArrayStringUnionPtr signature,
      const absl::optional<std::string>& error) override;
  void GetPendingAddSuggestTokenRequests(
      GetPendingAddSuggestTokenRequestsCallback callback) override;
  void GetPendingGetEncryptionPublicKeyRequests(
      GetPendingGetEncryptionPublicKeyRequestsCallback callback) override;
  void GetPendingDecryptRequests(
      GetPendingDecryptRequestsCallback callback) override;
  void NotifyAddSuggestTokenRequestsProcessed(
      bool approved,
      const std::vector<std::string>& contract_addresses) override;
  void NotifyGetPublicKeyRequestProcessed(bool approved,
                                          const url::Origin& origin) override;
  void NotifyDecryptRequestProcessed(bool approved,
                                     const url::Origin& origin) override;

  void IsBase58EncodedSolanaPubkey(
      const std::string& key,
      IsBase58EncodedSolanaPubkeyCallback callback) override;

  void Base58Encode(const std::vector<std::vector<std::uint8_t>>& addresses,
                    Base58EncodeCallback callback) override;

  // BraveWalletServiceDelegate::Observer:
  void OnActiveOriginChanged(const mojom::OriginInfoPtr& origin_info) override;

  // Resets things back to the original state of BraveWalletService.
  // To be used when the Wallet is reset / erased
  void Reset() override;

  void AddSignMessageRequest(mojom::SignMessageRequestPtr request,
                             SignMessageRequestCallback callback);
  void AddSuggestTokenRequest(mojom::AddSuggestTokenRequestPtr request,
                              mojom::EthereumProvider::RequestCallback callback,
                              base::Value id);
  void AddGetPublicKeyRequest(const std::string& address,
                              const url::Origin& origin,
                              mojom::EthereumProvider::RequestCallback callback,
                              base::Value id);
  void AddDecryptRequest(mojom::DecryptRequestPtr request,
                         mojom::EthereumProvider::RequestCallback callback,
                         base::Value id);
  void AddSignTransactionRequest(mojom::SignTransactionRequestPtr request,
                                 SignTransactionRequestCallback callback);
  void AddSignAllTransactionsRequest(
      mojom::SignAllTransactionsRequestPtr request,
      SignAllTransactionsRequestCallback callback);

  void RemovePrefListenersForTests();

  void OnOnboardingShown() override;

 private:
  friend class EthereumProviderImplUnitTest;
  friend class SolanaProviderImplUnitTest;
  friend class BraveWalletServiceUnitTest;

  FRIEND_TEST_ALL_PREFIXES(BraveWalletServiceUnitTest, GetChecksumAddress);
  FRIEND_TEST_ALL_PREFIXES(BraveWalletServiceUnitTest, AddSuggestToken);
  FRIEND_TEST_ALL_PREFIXES(BraveWalletServiceUnitTest, GetUserAsset);
  FRIEND_TEST_ALL_PREFIXES(BraveWalletServiceUnitTest, ImportFromMetaMask);
  FRIEND_TEST_ALL_PREFIXES(BraveWalletServiceUnitTest, Reset);
  FRIEND_TEST_ALL_PREFIXES(BraveWalletServiceUnitTest, GetUserAssetAddress);

  void OnDefaultWalletChanged();
  void OnDefaultBaseCurrencyChanged();
  void OnDefaultBaseCryptocurrencyChanged();
  void OnNetworkListChanged();

  static absl::optional<std::string> GetChecksumAddress(
      const std::string& contract_address,
      const std::string& chain_id);
  static absl::optional<std::string> GetUserAssetAddress(
      const std::string& address,
      mojom::CoinType coin,
      const std::string& chain_id);
  void OnWalletUnlockPreferenceChanged(const std::string& pref_name);
  void OnP3ATimerFired();

  void RecordWalletUsage(bool unlocked);

  void WriteStatsToHistogram(base::Time wallet_last_used,
                             base::Time first_p3a_report,
                             base::Time last_p3a_report,
                             bool was_onboarding_shown,
                             unsigned use_days_in_week);

  void OnGetImportInfo(
      const std::string& new_password,
      base::OnceCallback<void(bool, const absl::optional<std::string>&)>
          callback,
      bool result,
      ImportInfo info,
      ImportError error);

  bool AddUserAsset(mojom::BlockchainTokenPtr token);
  bool RemoveUserAsset(mojom::BlockchainTokenPtr token);
  bool SetUserAssetVisible(mojom::BlockchainTokenPtr token, bool visible);
  mojom::BlockchainTokenPtr GetUserAsset(const std::string& contract_address,
                                         const std::string& token_id,
                                         bool is_erc721,
                                         const std::string& chain_id,
                                         mojom::CoinType coin);
  void OnNetworkChanged();
  void CancelAllSuggestedTokenCallbacks();
  void CancelAllSignMessageCallbacks();
  void CancelAllSignTransactionCallbacks();
  void CancelAllSignAllTransactionsCallbacks();
  void CancelAllGetEncryptionPublicKeyCallbacks();
  void CancelAllDecryptCallbacks();

  int sign_message_id_ = 0;
  int sign_transaction_id_ = 0;
  int sign_all_transactions_id_ = 0;
  base::circular_deque<mojom::SignMessageRequestPtr> sign_message_requests_;
  base::circular_deque<SignMessageRequestCallback> sign_message_callbacks_;
  base::circular_deque<mojom::SignTransactionRequestPtr>
      sign_transaction_requests_;
  base::circular_deque<SignTransactionRequestCallback>
      sign_transaction_callbacks_;
  base::circular_deque<mojom::SignAllTransactionsRequestPtr>
      sign_all_transactions_requests_;
  base::circular_deque<SignAllTransactionsRequestCallback>
      sign_all_transactions_callbacks_;
  base::flat_map<std::string, mojom::EthereumProvider::RequestCallback>
      add_suggest_token_callbacks_;
  base::flat_map<std::string, base::Value> add_suggest_token_ids_;
  base::flat_map<std::string, mojom::AddSuggestTokenRequestPtr>
      add_suggest_token_requests_;
  base::flat_map<url::Origin, std::string>
      add_get_encryption_public_key_requests_;
  base::flat_map<url::Origin, mojom::EthereumProvider::RequestCallback>
      add_get_encryption_public_key_callbacks_;
  base::flat_map<url::Origin, base::Value> get_encryption_public_key_ids_;
  base::flat_map<url::Origin, mojom::DecryptRequestPtr> decrypt_requests_;
  base::flat_map<url::Origin, mojom::EthereumProvider::RequestCallback>
      decrypt_callbacks_;
  base::flat_map<url::Origin, base::Value> decrypt_ids_;
  mojo::RemoteSet<mojom::BraveWalletServiceObserver> observers_;
  std::unique_ptr<BraveWalletServiceDelegate> delegate_;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  raw_ptr<TxService> tx_service_ = nullptr;
  raw_ptr<PrefService> prefs_ = nullptr;
  BraveWalletP3A brave_wallet_p3a_;
  mojo::ReceiverSet<mojom::BraveWalletService> receivers_;
  PrefChangeRegistrar pref_change_registrar_;
  base::RepeatingTimer p3a_periodic_timer_;
  base::WeakPtrFactory<BraveWalletService> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_
