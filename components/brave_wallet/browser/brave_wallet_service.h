/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/circular_deque.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_wallet/browser/asset_discovery_manager.h"
#include "brave/components/brave_wallet/browser/brave_wallet_p3a.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/browser/simple_hash_client.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/origin.h"

namespace network {
class SharedURLLoaderFactory;
}

class PrefService;

namespace brave_wallet {

extern const char kBraveWalletWeeklyHistogramName[];
extern const char kBraveWalletMonthlyHistogramName[];
extern const char kBraveWalletNewUserReturningHistogramName[];
extern const char kBraveWalletLastUsageTimeHistogramName[];

class KeyringService;
class JsonRpcService;
class TxService;
class EthAllowanceManager;
struct PendingDecryptRequest;
struct PendingGetEncryptPublicKeyRequest;

class BraveWalletService : public KeyedService,
                           public mojom::BraveWalletService,
                           public BraveWalletServiceDelegate::Observer {
 public:
  using APIRequestHelper = api_request_helper::APIRequestHelper;
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

  BraveWalletService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::unique_ptr<BraveWalletServiceDelegate> delegate,
      KeyringService* keyring_service,
      JsonRpcService* json_rpc_service,
      TxService* tx_service,
      PrefService* profile_prefs,
      PrefService* local_state);

  ~BraveWalletService() override;

  BraveWalletService(const BraveWalletService&) = delete;
  BraveWalletService& operator=(const BraveWalletService&) = delete;

  mojo::PendingRemote<mojom::BraveWalletService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::BraveWalletService> receiver);

  static void MigrateUserAssetEthContractAddress(PrefService* profile_prefs);
  static void MigrateMultichainUserAssets(PrefService* profile_prefs);
  static void MigrateUserAssetsAddPreloadingNetworks(
      PrefService* profile_prefs);
  static void MigrateUserAssetsAddIsNFT(PrefService* profile_prefs);
  static void MigrateHiddenNetworks(PrefService* profile_prefs);
  static void MigrateUserAssetsAddIsERC1155(PrefService* profile_prefs);
  static void MigrateUserAssetsAddIsSpam(PrefService* profile_prefs);
  static void MigrateFantomMainnetAsCustomNetwork(PrefService* prefs);
  static void MigrateCeloMainnetAsCustomNetwork(PrefService* prefs);

  static bool AddUserAsset(mojom::BlockchainTokenPtr token,
                           bool visible,
                           PrefService* profile_prefs);
  static std::vector<mojom::BlockchainTokenPtr> GetUserAssets(
      const std::string& chain_id,
      mojom::CoinType coin,
      PrefService* profile_prefs);
  static std::vector<mojom::BlockchainTokenPtr> GetUserAssets(
      PrefService* profile_prefs);
  static base::Value::Dict GetDefaultEthereumAssets();
  static base::Value::Dict GetDefaultSolanaAssets();
  static base::Value::Dict GetDefaultFilecoinAssets();
  static base::Value::Dict GetDefaultBitcoinAssets();

  // mojom::BraveWalletService:
  void AddObserver(::mojo::PendingRemote<mojom::BraveWalletServiceObserver>
                       observer) override;
  void AddTokenObserver(
      ::mojo::PendingRemote<mojom::BraveWalletServiceTokenObserver> observer)
      override;

  void GetUserAssets(const std::string& chain_id,
                     mojom::CoinType coin,
                     GetUserAssetsCallback callback) override;
  void GetAllUserAssets(GetUserAssetsCallback callback) override;
  void AddUserAsset(mojom::BlockchainTokenPtr token,
                    AddUserAssetCallback callback) override;
  void OnGetEthNftStandard(mojom::BlockchainTokenPtr token,
                           AddUserAssetCallback callback,
                           const absl::optional<std::string>& standard,
                           mojom::ProviderError error,
                           const std::string& error_message);
  void RemoveUserAsset(mojom::BlockchainTokenPtr token,
                       RemoveUserAssetCallback callback) override;
  void SetUserAssetVisible(mojom::BlockchainTokenPtr token,
                           bool visible,
                           SetUserAssetVisibleCallback callback) override;
  void SetAssetSpamStatus(mojom::BlockchainTokenPtr token,
                          bool is_spam,
                          SetAssetSpamStatusCallback callback) override;
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

  void GetDefaultEthereumWallet(
      GetDefaultEthereumWalletCallback callback) override;
  void GetDefaultSolanaWallet(GetDefaultSolanaWalletCallback callback) override;
  void SetDefaultEthereumWallet(mojom::DefaultWallet default_wallet) override;
  void SetDefaultSolanaWallet(mojom::DefaultWallet default_wallet) override;
  void GetDefaultBaseCurrency(GetDefaultBaseCurrencyCallback callback) override;
  void SetDefaultBaseCurrency(const std::string& currency) override;
  void GetDefaultBaseCryptocurrency(
      GetDefaultBaseCryptocurrencyCallback callback) override;
  void SetDefaultBaseCryptocurrency(const std::string& cryptocurrency) override;
  void EnsureSelectedAccountForChain(
      mojom::CoinType coin,
      const std::string& chain_id,
      EnsureSelectedAccountForChainCallback callback) override;
  mojom::AccountIdPtr EnsureSelectedAccountForChainSync(
      mojom::CoinType coin,
      const std::string& chain_id);
  void GetNetworkForSelectedAccountOnActiveOrigin(
      GetNetworkForSelectedAccountOnActiveOriginCallback callback) override;
  mojom::NetworkInfoPtr GetNetworkForSelectedAccountOnActiveOriginSync();
  void SetNetworkForSelectedAccountOnActiveOrigin(
      const std::string& chain_id,
      SetNetworkForSelectedAccountOnActiveOriginCallback callback) override;
  void AddPermission(mojom::AccountIdPtr account_id,
                     AddPermissionCallback callback) override;
  void HasPermission(std::vector<mojom::AccountIdPtr> accounts,
                     HasPermissionCallback callback) override;
  void ResetPermission(mojom::AccountIdPtr account_id,
                       ResetPermissionCallback callback) override;
  void IsPermissionDenied(mojom::CoinType coin,
                          IsPermissionDeniedCallback callback) override;
  void GetWebSitesWithPermission(
      mojom::CoinType coin,
      GetWebSitesWithPermissionCallback callback) override;
  void ResetWebSitePermission(mojom::CoinType coin,
                              const std::string& formed_website,
                              ResetWebSitePermissionCallback callback) override;
  void GetActiveOrigin(GetActiveOriginCallback callback) override;
  mojom::OriginInfoPtr GetActiveOriginSync();
  void GetPendingSignMessageRequests(
      GetPendingSignMessageRequestsCallback callback) override;
  void GetPendingSignMessageErrors(
      GetPendingSignMessageErrorsCallback callback) override;
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
  void NotifySignMessageErrorProcessed(const std::string& error_id) override;
  void GetPendingAddSuggestTokenRequests(
      GetPendingAddSuggestTokenRequestsCallback callback) override;
  void GetPendingGetEncryptionPublicKeyRequests(
      GetPendingGetEncryptionPublicKeyRequestsCallback callback) override;
  void GetPendingDecryptRequests(
      GetPendingDecryptRequestsCallback callback) override;
  void NotifyAddSuggestTokenRequestsProcessed(
      bool approved,
      const std::vector<std::string>& contract_addresses) override;
  void NotifyGetPublicKeyRequestProcessed(const std::string& request_id,
                                          bool approved) override;
  void NotifyDecryptRequestProcessed(const std::string& request_id,
                                     bool approved) override;

  void IsBase58EncodedSolanaPubkey(
      const std::string& key,
      IsBase58EncodedSolanaPubkeyCallback callback) override;

  void Base58Encode(const std::vector<std::vector<std::uint8_t>>& addresses,
                    Base58EncodeCallback callback) override;

  void DiscoverAssetsOnAllSupportedChains() override;

  void GetNftDiscoveryEnabled(GetNftDiscoveryEnabledCallback callback) override;

  void SetNftDiscoveryEnabled(bool enabled) override;

  void GetBalanceScannerSupportedChains(
      GetBalanceScannerSupportedChainsCallback callback) override;

  void GetSimpleHashSpamNFTs(const std::string& wallet_address,
                             const std::vector<std::string>& chain_ids,
                             mojom::CoinType coin,
                             const absl::optional<std::string>& cursor,
                             GetSimpleHashSpamNFTsCallback callback) override;

  void ConvertFEVMToFVMAddress(
      bool is_mainnet,
      const std::vector<std::string>& fevm_addresses,
      ConvertFEVMToFVMAddressCallback callback) override;

  // BraveWalletServiceDelegate::Observer:
  void OnActiveOriginChanged(const mojom::OriginInfoPtr& origin_info) override;

  void OnDiscoverAssetsStarted();

  void OnDiscoverAssetsCompleted(
      std::vector<mojom::BlockchainTokenPtr> discovered_assets);

  // Resets things back to the original state of BraveWalletService.
  // To be used when the Wallet is reset / erased
  void Reset() override;

  void DiscoverEthAllowances(DiscoverEthAllowancesCallback callback) override;

  void AddSignMessageRequest(mojom::SignMessageRequestPtr request,
                             SignMessageRequestCallback callback);
  void AddSignMessageError(mojom::SignMessageErrorPtr error);
  void AddSuggestTokenRequest(mojom::AddSuggestTokenRequestPtr request,
                              mojom::EthereumProvider::RequestCallback callback,
                              base::Value id);
  void AddGetPublicKeyRequest(const std::string& address,
                              const url::Origin& origin,
                              mojom::EthereumProvider::RequestCallback callback,
                              base::Value id);
  void AddDecryptRequest(const url::Origin& origin,
                         const std::string& address,
                         std::string unsafe_message,
                         mojom::EthereumProvider::RequestCallback callback,
                         base::Value id);
  void AddSignTransactionRequest(mojom::SignTransactionRequestPtr request,
                                 SignTransactionRequestCallback callback);
  void AddSignAllTransactionsRequest(
      mojom::SignAllTransactionsRequestPtr request,
      SignAllTransactionsRequestCallback callback);

  void RemovePrefListenersForTests();

  BraveWalletP3A* GetBraveWalletP3A();

  void SetSignTransactionRequestAddedCallbackForTesting(
      base::OnceClosure callback) {
    sign_tx_request_added_cb_for_testing_ = std::move(callback);
  }
  void SetSignAllTransactionsRequestAddedCallbackForTesting(
      base::OnceClosure callback) {
    sign_all_txs_request_added_cb_for_testing_ = std::move(callback);
  }

 protected:
  // For tests
  BraveWalletService();

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

  bool HasPendingDecryptRequestForOrigin(const url::Origin& origin) const;
  bool HasPendingGetEncryptionPublicKeyRequestForOrigin(
      const url::Origin& origin) const;

  void OnDefaultEthereumWalletChanged();
  void OnDefaultSolanaWalletChanged();
  void OnDefaultBaseCurrencyChanged();
  void OnDefaultBaseCryptocurrencyChanged();
  void OnNetworkListChanged();
  void OnBraveWalletNftDiscoveryEnabled();

  static absl::optional<std::string> GetChecksumAddress(
      const std::string& contract_address,
      const std::string& chain_id);
  static absl::optional<std::string> GetUserAssetAddress(
      const std::string& address,
      mojom::CoinType coin,
      const std::string& chain_id);
  void OnWalletUnlockPreferenceChanged(const std::string& pref_name);

  void OnGetImportInfo(
      const std::string& new_password,
      base::OnceCallback<void(bool, const absl::optional<std::string>&)>
          callback,
      bool result,
      ImportInfo info,
      ImportError error);

  bool AddUserAsset(mojom::BlockchainTokenPtr token, bool visible = true);
  bool RemoveUserAsset(mojom::BlockchainTokenPtr token);
  bool SetUserAssetVisible(mojom::BlockchainTokenPtr token, bool visible);
  bool SetAssetSpamStatus(mojom::BlockchainTokenPtr token, bool is_spam);
  mojom::BlockchainTokenPtr GetUserAsset(const std::string& contract_address,
                                         const std::string& token_id,
                                         bool is_nft,
                                         const std::string& chain_id,
                                         mojom::CoinType coin);
  void OnNetworkChanged();
  void CancelAllSuggestedTokenCallbacks();
  void CancelAllSignMessageCallbacks();
  void CancelAllSignTransactionCallbacks();
  void CancelAllSignAllTransactionsCallbacks();
  void CancelAllGetEncryptionPublicKeyCallbacks();
  void CancelAllDecryptCallbacks();
  void DiscoverAssetsOnAllSupportedChains(bool bypass_rate_limit);

  base::OnceClosure sign_tx_request_added_cb_for_testing_;
  base::OnceClosure sign_all_txs_request_added_cb_for_testing_;

  int sign_message_id_ = 0;
  int sign_transaction_id_ = 0;
  int sign_all_transactions_id_ = 0;
  base::circular_deque<mojom::SignMessageRequestPtr> sign_message_requests_;
  base::circular_deque<SignMessageRequestCallback> sign_message_callbacks_;
  base::circular_deque<mojom::SignMessageErrorPtr> sign_message_errors_;
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
  base::flat_map<std::string, PendingGetEncryptPublicKeyRequest>
      pending_get_encryption_public_key_requests_;
  base::flat_map<std::string, PendingDecryptRequest> pending_decrypt_requests_;
  mojo::RemoteSet<mojom::BraveWalletServiceObserver> observers_;
  mojo::RemoteSet<mojom::BraveWalletServiceTokenObserver> token_observers_;
  std::unique_ptr<BraveWalletServiceDelegate> delegate_;
  raw_ptr<KeyringService> keyring_service_ = nullptr;
  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;
  raw_ptr<TxService> tx_service_ = nullptr;
  raw_ptr<PrefService> profile_prefs_ = nullptr;
  BraveWalletP3A brave_wallet_p3a_;
  std::unique_ptr<SimpleHashClient> simple_hash_client_;
  std::unique_ptr<AssetDiscoveryManager> asset_discovery_manager_;
  std::unique_ptr<EthAllowanceManager> eth_allowance_manager_;
  mojo::ReceiverSet<mojom::BraveWalletService> receivers_;
  PrefChangeRegistrar pref_change_registrar_;
  base::WeakPtrFactory<BraveWalletService> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_
