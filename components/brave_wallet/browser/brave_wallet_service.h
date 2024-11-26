/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/containers/circular_deque.h"
#include "base/gtest_prod_util.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/brave_wallet/browser/asset_discovery_manager.h"
#include "brave/components/brave_wallet/browser/bitcoin/bitcoin_wallet_service.h"
#include "brave/components/brave_wallet/browser/brave_wallet_p3a.h"
#include "brave/components/brave_wallet/browser/brave_wallet_service_delegate.h"
#include "brave/components/brave_wallet/browser/simple_hash_client.h"
#include "brave/components/brave_wallet/browser/zcash/zcash_wallet_service.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "components/prefs/pref_change_registrar.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "url/origin.h"

namespace network {
class SharedURLLoaderFactory;
}

class PrefService;

namespace brave_wallet {

class EthAllowanceManager;
class JsonRpcService;
class KeyringService;
class NetworkManager;
class TxService;
class AccountDiscoveryManager;
struct PendingDecryptRequest;
struct PendingGetEncryptPublicKeyRequest;

class BraveWalletService : public KeyedService,
                           public mojom::BraveWalletService,
                           public KeyringServiceObserverBase,
                           public BraveWalletServiceDelegate::Observer {
 public:
  using APIRequestHelper = api_request_helper::APIRequestHelper;
  using SignMessageRequestCallback =
      base::OnceCallback<void(bool,
                              mojom::EthereumSignatureBytesPtr,
                              const std::optional<std::string>&)>;
  using SignSolTransactionsRequestCallback =
      base::OnceCallback<void(bool,
                              std::vector<mojom::SolanaSignaturePtr>,
                              const std::optional<std::string>&)>;
  using AddSuggestTokenCallback =
      base::OnceCallback<void(bool, mojom::ProviderError, const std::string&)>;

  BraveWalletService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      std::unique_ptr<BraveWalletServiceDelegate> delegate,
      PrefService* profile_prefs,
      PrefService* local_state);

  ~BraveWalletService() override;

  BraveWalletService(const BraveWalletService&) = delete;
  BraveWalletService& operator=(const BraveWalletService&) = delete;

  template <class T>
  void Bind(mojo::PendingReceiver<T> receiver);

  static void MigrateHiddenNetworks(PrefService* profile_prefs);
  static void MigrateDeadNetwork(PrefService* prefs,
                                 const std::string& chain_id,
                                 const std::string& fallback_chain_id,
                                 const char* pref_key);
  static void MigrateAsCustomNetwork(PrefService* prefs,
                                     const mojom::NetworkInfo& network,
                                     bool is_eip1559,
                                     const char* pref_key);
  static void MigrateFantomMainnetAsCustomNetwork(PrefService* prefs);
  static void MigrateGoerliNetwork(PrefService* prefs);
  static void MigrateAuroraMainnetAsCustomNetwork(PrefService* prefs);
  static void MigrateAssetsPrefToList(PrefService* prefs);
  static void MigrateEip1559ForCustomNetworks(PrefService* prefs);
  void MaybeMigrateCompressedNfts();
  void MaybeMigrateSPLTokenProgram();

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
  void GetPendingSignSolTransactionsRequests(
      GetPendingSignSolTransactionsRequestsCallback callback) override;
  void NotifySignSolTransactionsRequestProcessed(
      bool approved,
      int id,
      std::vector<mojom::SolanaSignaturePtr> hw_signatures,
      const std::optional<std::string>& error) override;
  void NotifySignMessageRequestProcessed(
      bool approved,
      int id,
      mojom::EthereumSignatureBytesPtr hw_signature,
      const std::optional<std::string>& error) override;
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

  void DiscoverAssetsOnAllSupportedChains(bool bypass_rate_limit) override;

  void GetNftDiscoveryEnabled(GetNftDiscoveryEnabledCallback callback) override;

  void SetNftDiscoveryEnabled(bool enabled) override;

  void GetPrivateWindowsEnabled(
      GetPrivateWindowsEnabledCallback callback) override;

  void SetPrivateWindowsEnabled(bool enabled) override;

  void GetBalanceScannerSupportedChains(
      GetBalanceScannerSupportedChainsCallback callback) override;

  void GetSimpleHashSpamNFTs(const std::string& wallet_address,
                             const std::vector<std::string>& chain_ids,
                             mojom::CoinType coin,
                             const std::optional<std::string>& cursor,
                             GetSimpleHashSpamNFTsCallback callback) override;

  void ConvertFEVMToFVMAddress(
      bool is_mainnet,
      const std::vector<std::string>& fevm_addresses,
      ConvertFEVMToFVMAddressCallback callback) override;

  void GenerateReceiveAddress(mojom::AccountIdPtr account_id,
                              GenerateReceiveAddressCallback callback) override;

  void GetAnkrSupportedChainIds(
      GetAnkrSupportedChainIdsCallback callback) override;

  void IsPrivateWindow(IsPrivateWindowCallback callback) override;

  void GetTransactionSimulationOptInStatus(
      GetTransactionSimulationOptInStatusCallback callback) override;
  mojom::BlowfishOptInStatus GetTransactionSimulationOptInStatusSync();

  void SetTransactionSimulationOptInStatus(
      mojom::BlowfishOptInStatus status) override;

  // BraveWalletServiceDelegate::Observer:
  void OnActiveOriginChanged(const mojom::OriginInfoPtr& origin_info) override;

  // KeyringServiceObserverBase:
  void WalletRestored() override;

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
  void AddGetPublicKeyRequest(const mojom::AccountIdPtr& account_id,
                              const url::Origin& origin,
                              mojom::EthereumProvider::RequestCallback callback,
                              base::Value id);
  void AddDecryptRequest(const mojom::AccountIdPtr& account_id,
                         const url::Origin& origin,
                         std::string unsafe_message,
                         mojom::EthereumProvider::RequestCallback callback,
                         base::Value id);
  void AddSignSolTransactionsRequest(
      mojom::SignSolTransactionsRequestPtr request,
      SignSolTransactionsRequestCallback callback);
  mojom::SignSolTransactionsRequestPtr GetPendingSignSolTransactionsRequest(
      int32_t id);

  void RemovePrefListenersForTests();

  BraveWalletP3A* GetBraveWalletP3A();

  void SetSignSolTransactionsRequestAddedCallbackForTesting(
      base::OnceClosure callback) {
    sign_sol_txs_request_added_cb_for_testing_ = std::move(callback);
  }

  BraveWalletServiceDelegate* GetDelegateForTesting() {
    return delegate_.get();
  }

  NetworkManager* network_manager() { return network_manager_.get(); }
  JsonRpcService* json_rpc_service() { return json_rpc_service_.get(); }
  KeyringService* keyring_service() { return keyring_service_.get(); }
  TxService* tx_service() { return tx_service_.get(); }
  // Might return nullptr.
  BitcoinWalletService* GetBitcoinWalletService();
  // Might return nullptr.
  ZCashWalletService* GetZcashWalletService();

  void GetCountryCode(GetCountryCodeCallback callback) override;

 protected:
  // For tests
  BraveWalletService();

 private:
  friend class EthereumProviderImplUnitTest;
  friend class SolanaProviderImplUnitTest;
  friend class BraveWalletServiceUnitTest;

  FRIEND_TEST_ALL_PREFIXES(BraveWalletServiceUnitTest, AddSuggestToken);
  FRIEND_TEST_ALL_PREFIXES(BraveWalletServiceUnitTest, ImportFromMetaMask);
  FRIEND_TEST_ALL_PREFIXES(BraveWalletServiceUnitTest, Reset);

  bool HasPendingDecryptRequestForOrigin(const url::Origin& origin) const;
  bool HasPendingGetEncryptionPublicKeyRequestForOrigin(
      const url::Origin& origin) const;

  void OnDefaultEthereumWalletChanged();
  void OnDefaultSolanaWalletChanged();
  void OnDefaultBaseCurrencyChanged();
  void OnDefaultBaseCryptocurrencyChanged();
  void OnNetworkListChanged();
  void OnBraveWalletNftDiscoveryEnabled();

  void OnGenerateBtcReceiveAddress(
      GenerateReceiveAddressCallback callback,
      mojom::BitcoinAddressPtr,
      const std::optional<std::string>& error_message);

  void OnGenerateZecReceiveAddress(
      GenerateReceiveAddressCallback callback,
      base::expected<mojom::ZCashAddressPtr, std::string> result);

  void OnWalletUnlockPreferenceChanged(const std::string& pref_name);

  void OnGetImportInfo(
      const std::string& new_password,
      base::OnceCallback<void(bool, const std::optional<std::string>&)>
          callback,
      base::expected<ImportInfo, ImportError> info);
  void OnGetEthNftStandard(mojom::BlockchainTokenPtr token,
                           AddUserAssetCallback callback,
                           const std::optional<std::string>& standard,
                           mojom::ProviderError error,
                           const std::string& error_message);

  void OnGetNfts(AddUserAssetCallback callback,
                 std::vector<mojom::BlockchainTokenPtr> nfts);
  bool AddUserAssetInternal(mojom::BlockchainTokenPtr token);
  bool RemoveUserAsset(mojom::BlockchainTokenPtr token);
  bool SetUserAssetVisible(mojom::BlockchainTokenPtr token, bool visible);
  bool SetAssetSpamStatus(mojom::BlockchainTokenPtr token, bool is_spam);
  void OnNetworkChanged();
  void CancelAllSuggestedTokenCallbacks();
  void CancelAllSignMessageCallbacks();
  void CancelAllSignSolTransactionsCallbacks();
  void CancelAllGetEncryptionPublicKeyCallbacks();
  void CancelAllDecryptCallbacks();
  void OnGetNftsForCompressedMigration(
      std::vector<mojom::BlockchainTokenPtr> nfts);

  base::OnceClosure sign_tx_request_added_cb_for_testing_;
  base::OnceClosure sign_sol_txs_request_added_cb_for_testing_;

  int sign_message_id_ = 0;
  int sign_sol_transactions_id_ = 0;
  base::circular_deque<mojom::SignMessageRequestPtr> sign_message_requests_;
  base::circular_deque<SignMessageRequestCallback> sign_message_callbacks_;
  base::circular_deque<mojom::SignMessageErrorPtr> sign_message_errors_;
  base::circular_deque<mojom::SignSolTransactionsRequestPtr>
      sign_sol_transactions_requests_;
  base::circular_deque<SignSolTransactionsRequestCallback>
      sign_sol_transactions_callbacks_;
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
  std::unique_ptr<NetworkManager> network_manager_;
  std::unique_ptr<JsonRpcService> json_rpc_service_;
  std::unique_ptr<KeyringService> keyring_service_;
  std::unique_ptr<BitcoinWalletService> bitcoin_wallet_service_;
  std::unique_ptr<ZCashWalletService> zcash_wallet_service_;
  std::unique_ptr<TxService> tx_service_;
  raw_ptr<PrefService> profile_prefs_ = nullptr;
  std::unique_ptr<BraveWalletP3A> brave_wallet_p3a_;
  std::unique_ptr<SimpleHashClient> simple_hash_client_;
  std::unique_ptr<AssetDiscoveryManager> asset_discovery_manager_;
  std::unique_ptr<EthAllowanceManager> eth_allowance_manager_;
  std::unique_ptr<AccountDiscoveryManager> account_discovery_manager_;
  mojo::ReceiverSet<mojom::BraveWalletService> receivers_;
  mojo::Receiver<brave_wallet::mojom::KeyringServiceObserver>
      keyring_observer_receiver_{this};
  PrefChangeRegistrar pref_change_registrar_;
  base::WeakPtrFactory<BraveWalletService> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BRAVE_WALLET_SERVICE_H_
