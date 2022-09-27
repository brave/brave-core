/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_SERVICE_H_

#include <list>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list_threadsafe.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/ens_resolver_task.h"
#include "brave/components/brave_wallet/browser/solana_transaction.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "services/data_decoder/public/cpp/json_sanitizer.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

class PrefService;

namespace brave_wallet {

class EnsResolverTask;

namespace unstoppable_domains {
template <class ResultType>
class MultichainCalls;
}  // namespace unstoppable_domains

class JsonRpcService : public KeyedService, public mojom::JsonRpcService {
 public:
  JsonRpcService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* prefs,
      PrefService* local_state_prefs);
  // For testing:
  JsonRpcService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* prefs);
  ~JsonRpcService() override;

  static void MigrateMultichainNetworks(PrefService* prefs);

  mojo::PendingRemote<mojom::JsonRpcService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::JsonRpcService> receiver);

  using APIRequestHelper = api_request_helper::APIRequestHelper;
  using APIRequestResult = api_request_helper::APIRequestResult;
  using StringResultCallback =
      base::OnceCallback<void(const std::string& result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  using EnsGetContentHashCallback =
      base::OnceCallback<void(const std::vector<uint8_t>& content_hash,
                              bool require_offchain_consent,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  using GetBlockNumberCallback =
      base::OnceCallback<void(uint256_t result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  using RequestIntermediateCallback =
      base::OnceCallback<void(APIRequestResult api_request_result)>;
  using GetFeeHistoryCallback = base::OnceCallback<void(
      const std::vector<std::string>& base_fee_per_gas,
      const std::vector<double>& gas_used_ratio,
      const std::string& oldest_block,
      const std::vector<std::vector<std::string>>& reward,
      mojom::ProviderError error,
      const std::string& error_message)>;
  void GetBlockNumber(GetBlockNumberCallback callback);
  void GetFeeHistory(GetFeeHistoryCallback callback);

  void Request(const std::string& json_payload,
               bool auto_retry_on_network_change,
               base::Value id,
               mojom::CoinType coin,
               RequestCallback callback) override;

  void OnRequestResult(RequestCallback callback,
                       base::Value id,
                       APIRequestResult api_request_result);

  void GetBalance(const std::string& address,
                  mojom::CoinType coin,
                  const std::string& chaind_id,
                  GetBalanceCallback callback) override;
  using GetFilBlockHeightCallback =
      base::OnceCallback<void(uint64_t height,
                              mojom::FilecoinProviderError error,
                              const std::string& error_message)>;
  void GetFilBlockHeight(GetFilBlockHeightCallback callback);
  using GetFilStateSearchMsgLimitedCallback =
      base::OnceCallback<void(int64_t code,
                              mojom::FilecoinProviderError error,
                              const std::string& error_message)>;
  void GetFilStateSearchMsgLimited(
      const std::string& cid,
      uint64_t period,
      GetFilStateSearchMsgLimitedCallback callback);

  using GetTxCountCallback =
      base::OnceCallback<void(uint256_t result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  using GetFilTxCountCallback =
      base::OnceCallback<void(uint256_t result,
                              mojom::FilecoinProviderError error,
                              const std::string& error_message)>;
  void GetEthTransactionCount(const std::string& address,
                              GetTxCountCallback callback);
  void GetFilTransactionCount(const std::string& address,
                              GetFilTxCountCallback callback);
  using SendFilecoinTransactionCallback =
      base::OnceCallback<void(const std::string& tx_hash,
                              mojom::FilecoinProviderError error,
                              const std::string& error_message)>;
  void SendFilecoinTransaction(const std::string& signed_tx,
                               SendFilecoinTransactionCallback callback);

  using GetTxReceiptCallback =
      base::OnceCallback<void(TransactionReceipt result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  void GetTransactionReceipt(const std::string& tx_hash,
                             GetTxReceiptCallback callback);

  using SendRawTxCallback =
      base::OnceCallback<void(const std::string& tx_hash,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  void SendRawTransaction(const std::string& signed_tx,
                          SendRawTxCallback callback);

  void GetERC20TokenBalance(const std::string& conract_address,
                            const std::string& address,
                            const std::string& chain_id,
                            GetERC20TokenBalanceCallback callback) override;
  void GetERC20TokenAllowance(const std::string& contract_address,
                              const std::string& owner_address,
                              const std::string& spender_address,
                              GetERC20TokenAllowanceCallback callback) override;

  using UnstoppableDomainsResolveDnsCallback =
      base::OnceCallback<void(const GURL& url,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  void UnstoppableDomainsResolveDns(
      const std::string& domain,
      UnstoppableDomainsResolveDnsCallback callback);

  void UnstoppableDomainsGetEthAddr(
      const std::string& domain,
      UnstoppableDomainsGetEthAddrCallback callback) override;

  void EnsGetContentHash(const std::string& domain,
                         EnsGetContentHashCallback callback);
  void EnsGetEthAddr(const std::string& domain,
                     mojom::EnsOffchainLookupOptionsPtr options,
                     EnsGetEthAddrCallback callback) override;

  bool SetNetwork(const std::string& chain_id,
                  mojom::CoinType coin,
                  bool silent = false);
  void SetNetwork(const std::string& chain_id,
                  mojom::CoinType coin,
                  SetNetworkCallback callback) override;
  void GetNetwork(mojom::CoinType coin, GetNetworkCallback callback) override;
  void AddChain(mojom::NetworkInfoPtr chain,
                AddChainCallback callback) override;
  void AddEthereumChainForOrigin(
      mojom::NetworkInfoPtr chain,
      const url::Origin& origin,
      AddEthereumChainForOriginCallback callback) override;
  void AddEthereumChainRequestCompleted(const std::string& chain_id,
                                        bool approved) override;
  void RemoveChain(const std::string& chain_id,
                   mojom::CoinType coin,
                   RemoveChainCallback callback) override;

  std::string GetChainId(mojom::CoinType coin) const;
  void GetChainId(mojom::CoinType coin,
                  mojom::JsonRpcService::GetChainIdCallback callback) override;
  void GetBlockTrackerUrl(
      mojom::JsonRpcService::GetBlockTrackerUrlCallback callback) override;
  void GetPendingAddChainRequests(
      GetPendingAddChainRequestsCallback callback) override;
  void GetPendingSwitchChainRequests(
      GetPendingSwitchChainRequestsCallback callback) override;
  void NotifySwitchChainRequestProcessed(bool approved,
                                         const url::Origin& origin) override;
  void GetAllNetworks(mojom::CoinType coin,
                      GetAllNetworksCallback callback) override;
  void GetCustomNetworks(mojom::CoinType coin,
                         GetCustomNetworksCallback callback) override;
  void GetKnownNetworks(mojom::CoinType coin,
                        GetKnownNetworksCallback callback) override;
  void GetHiddenNetworks(mojom::CoinType coin,
                         GetHiddenNetworksCallback callback) override;
  std::string GetNetworkUrl(mojom::CoinType coin) const;
  void GetNetworkUrl(
      mojom::CoinType coin,
      mojom::JsonRpcService::GetNetworkUrlCallback callback) override;
  void SetCustomNetworkForTesting(const std::string& chain_id,
                                  mojom::CoinType coin,
                                  const GURL& provider_url) override;

  void AddObserver(
      ::mojo::PendingRemote<mojom::JsonRpcServiceObserver> observer) override;

  GURL GetBlockTrackerUrlFromNetwork(const std::string& chain_id);
  using GetFilEstimateGasCallback =
      base::OnceCallback<void(const std::string& gas_premium,
                              const std::string& gas_fee_cap,
                              int64_t gas_limit,
                              mojom::FilecoinProviderError error,
                              const std::string& error_message)>;
  void GetFilEstimateGas(const std::string& from_address,
                         const std::string& to_address,
                         const std::string& gas_premium,
                         const std::string& gas_fee_cap,
                         int64_t gas_limit,
                         uint64_t nonce,
                         const std::string& max_fee,
                         const std::string& value,
                         GetFilEstimateGasCallback callback);

  using GetEstimateGasCallback =
      base::OnceCallback<void(const std::string& result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  void GetEstimateGas(const std::string& from_address,
                      const std::string& to_address,
                      const std::string& gas,
                      const std::string& gas_price,
                      const std::string& value,
                      const std::string& data,
                      GetEstimateGasCallback callback);

  using GetGasPriceCallback =
      base::OnceCallback<void(const std::string& result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  void GetGasPrice(GetGasPriceCallback callback);

  using GetIsEip1559Callback =
      base::OnceCallback<void(bool is_eip1559,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  void GetIsEip1559(GetIsEip1559Callback callback);

  void GetERC721OwnerOf(const std::string& contract,
                        const std::string& token_id,
                        const std::string& chain_id,
                        GetERC721OwnerOfCallback callback) override;

  void GetERC1155TokenBalance(const std::string& contract_address,
                              const std::string& owner_address,
                              const std::string& token_id,
                              const std::string& chain_id,
                              GetERC1155TokenBalanceCallback callback) override;

  void GetERC721TokenBalance(const std::string& contract_address,
                             const std::string& token_id,
                             const std::string& account_address,
                             const std::string& chain_id,
                             GetERC721TokenBalanceCallback callback) override;

  using GetTokenMetadataCallback =
      base::OnceCallback<void(const std::string& result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;

  void GetTokenMetadata(const std::string& contract_address,
                        const std::string& token_id,
                        const std::string& chain_id,
                        const std::string& interface_id,
                        GetTokenMetadataCallback callback);

  void GetERC721Metadata(const std::string& contract_address,
                         const std::string& token_id,
                         const std::string& chain_id,
                         GetTokenMetadataCallback callback) override;

  void GetERC1155Metadata(const std::string& contract_address,
                          const std::string& token_id,
                          const std::string& chain_id,
                          GetTokenMetadataCallback callback) override;

  using DiscoverAssetsCallback = base::OnceCallback<void(
      const std::vector<mojom::BlockchainTokenPtr> discovered_assets,
      mojom::ProviderError error,
      const std::string& error_message)>;
  void DiscoverAssets(const std::string& chain_id,
                      mojom::CoinType coin,
                      const std::vector<std::string>& account_addresses);

  void DiscoverAssetsInternal(const std::string& chain_id,
                              mojom::CoinType coin,
                              const std::vector<std::string>& account_addresses,
                              DiscoverAssetsCallback callback);

  void OnGetAllTokensDiscoverAssets(
      const std::string& chain_id,
      const std::vector<std::string>& account_addresses,
      std::vector<mojom::BlockchainTokenPtr> user_assets,
      DiscoverAssetsCallback callback,
      std::vector<mojom::BlockchainTokenPtr> token_list);

  void OnGetTransferLogs(
      DiscoverAssetsCallback callback,
      base::flat_map<std::string, mojom::BlockchainTokenPtr>& user_assets_map,
      APIRequestResult api_request_result);

  void OnDiscoverAssetsCompleted(
      std::vector<mojom::BlockchainTokenPtr> discovered_assets,
      mojom::ProviderError error,
      const std::string& error_message);

  // Resets things back to the original state of BraveWalletService.
  // To be used when the Wallet is reset / erased
  void Reset();

  using GetSupportsInterfaceCallback =
      base::OnceCallback<void(bool is_supported,
                              mojom::ProviderError error,
                              const std::string& error_message)>;

  void GetSupportsInterface(const std::string& contract_address,
                            const std::string& interface_id,
                            const std::string& chain_id,
                            GetSupportsInterfaceCallback callback);

  using SwitchEthereumChainRequestCallback =
      base::OnceCallback<void(mojom::ProviderError error,
                              const std::string& error_message)>;
  // return false when there is an error before processing request
  bool AddSwitchEthereumChainRequest(const std::string& chain_id,
                                     const url::Origin& origin,
                                     RequestCallback callback,
                                     base::Value id);

  void SetAPIRequestHelperForTesting(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  // Solana JSON RPCs
  void GetSolanaBalance(const std::string& pubkey,
                        const std::string& chain_id,
                        GetSolanaBalanceCallback callback) override;
  void GetSPLTokenAccountBalance(
      const std::string& wallet_address,
      const std::string& token_mint_address,
      const std::string& chain_id,
      GetSPLTokenAccountBalanceCallback callback) override;
  using SendSolanaTransactionCallback =
      base::OnceCallback<void(const std::string& tx_hash,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  void SendSolanaTransaction(
      const std::string& signed_tx,
      absl::optional<SolanaTransaction::SendOptions> send_options,
      SendSolanaTransactionCallback callback);
  using GetSolanaLatestBlockhashCallback =
      base::OnceCallback<void(const std::string& latest_blockhash,
                              uint64_t last_valid_block_height,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  void GetSolanaLatestBlockhash(GetSolanaLatestBlockhashCallback callback);
  using GetSolanaSignatureStatusesCallback = base::OnceCallback<void(
      const std::vector<absl::optional<SolanaSignatureStatus>>&
          signature_statuses,
      mojom::SolanaProviderError error,
      const std::string& error_message)>;
  void GetSolanaSignatureStatuses(const std::vector<std::string>& tx_signatures,
                                  GetSolanaSignatureStatusesCallback callback);
  using GetSolanaAccountInfoCallback =
      base::OnceCallback<void(absl::optional<SolanaAccountInfo> account_info,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  void GetSolanaAccountInfo(const std::string& pubkey,
                            GetSolanaAccountInfoCallback callback);
  using GetSolanaFeeForMessageCallback =
      base::OnceCallback<void(uint64_t fee,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  void GetSolanaFeeForMessage(const std::string& message,  // base64 encoded
                              GetSolanaFeeForMessageCallback callback);
  using GetSolanaBlockHeightCallback =
      base::OnceCallback<void(uint64_t block_height,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  void GetSolanaBlockHeight(GetSolanaBlockHeightCallback callback);

 private:
  void FireNetworkChanged(mojom::CoinType coin);
  void FirePendingRequestCompleted(const std::string& chain_id,
                                   const std::string& error);
  bool HasRequestFromOrigin(const url::Origin& origin) const;
  void RemoveChainIdRequest(const std::string& chain_id);
  void OnGetFilStateSearchMsgLimited(
      GetFilStateSearchMsgLimitedCallback callback,
      const std::string& cid,
      APIRequestResult api_request_result);
  void OnGetFilBlockHeight(GetFilBlockHeightCallback callback,
                           APIRequestResult api_request_result);
  void OnGetBlockNumber(GetBlockNumberCallback callback,
                        APIRequestResult api_request_result);
  void OnGetFeeHistory(GetFeeHistoryCallback callback,
                       APIRequestResult api_request_result);
  void OnEthGetBalance(GetBalanceCallback callback,
                       APIRequestResult api_request_result);
  void OnFilGetBalance(GetBalanceCallback callback,
                       APIRequestResult api_request_result);
  void OnEthGetTransactionCount(GetTxCountCallback callback,
                                APIRequestResult api_request_result);
  void OnFilGetTransactionCount(GetFilTxCountCallback callback,
                                APIRequestResult api_request_result);
  void OnSendFilecoinTransaction(SendFilecoinTransactionCallback callback,
                                 APIRequestResult api_request_result);
  void OnGetTransactionReceipt(GetTxReceiptCallback callback,
                               APIRequestResult api_request_result);
  void OnSendRawTransaction(SendRawTxCallback callback,
                            APIRequestResult api_request_result);
  void OnGetERC20TokenBalance(GetERC20TokenBalanceCallback callback,
                              APIRequestResult api_request_result);
  void OnGetERC20TokenAllowance(GetERC20TokenAllowanceCallback callback,
                                APIRequestResult api_request_result);
  void OnUnstoppableDomainsResolveDns(const std::string& domain,
                                      const std::string& chain_id,
                                      APIRequestResult api_request_result);
  void OnUnstoppableDomainsGetEthAddr(const std::string& domain,
                                      const std::string& chain_id,
                                      APIRequestResult api_request_result);
  void EnsRegistryGetResolver(const std::string& domain,
                              StringResultCallback callback);
  void OnEnsRegistryGetResolver(StringResultCallback callback,
                                APIRequestResult api_request_result);
  void ContinueEnsGetContentHash(const std::string& domain,
                                 EnsGetContentHashCallback callback,
                                 const std::string& resolver_address,
                                 mojom::ProviderError error,
                                 const std::string& error_message);
  void OnEnsGetContentHash(EnsGetContentHashCallback callback,
                           APIRequestResult api_request_result);
  void ContinueEnsGetEthAddr(const std::string& domain,
                             EnsGetEthAddrCallback callback,
                             const std::string& resolver_address,
                             mojom::ProviderError error,
                             const std::string& error_message);
  void OnEnsGetEthAddrTaskDone(
      EnsResolverTask* task,
      absl::optional<EnsResolverTaskResult> task_result,
      absl::optional<EnsResolverTaskError> error);
  void OnEnsGetContentHashTaskDone(
      EnsResolverTask* task,
      absl::optional<EnsResolverTaskResult> task_result,
      absl::optional<EnsResolverTaskError> error);
  void OnEnsGetEthAddr(EnsGetEthAddrCallback callback,
                       APIRequestResult api_request_result);
  void OnGetFilEstimateGas(GetFilEstimateGasCallback callback,
                           APIRequestResult api_request_result);
  void OnGetEstimateGas(GetEstimateGasCallback callback,
                        APIRequestResult api_request_result);
  void OnGetGasPrice(GetGasPriceCallback callback,
                     APIRequestResult api_request_result);
  void OnGetIsEip1559(GetIsEip1559Callback callback,
                      APIRequestResult api_request_result);

  void MaybeUpdateIsEip1559(const std::string& chain_id);
  void UpdateIsEip1559(const std::string& chain_id,
                       bool is_eip1559,
                       mojom::ProviderError error,
                       const std::string& error_message);

  void RequestInternal(
      const std::string& json_payload,
      bool auto_retry_on_network_change,
      const GURL& network_url,
      RequestIntermediateCallback callback,
      APIRequestHelper::ResponseConversionCallback conversion_callback);
  void OnEthChainIdValidatedForOrigin(const std::string& chain_id,
                                      const GURL& rpc_url,
                                      APIRequestResult api_request_result);

  void OnEthChainIdValidated(mojom::NetworkInfoPtr chain,
                             const GURL& rpc_url,
                             AddChainCallback callback,
                             APIRequestResult api_request_result);

  FRIEND_TEST_ALL_PREFIXES(JsonRpcServiceUnitTest, IsValidDomain);
  FRIEND_TEST_ALL_PREFIXES(JsonRpcServiceUnitTest, IsValidUnstoppableDomain);
  FRIEND_TEST_ALL_PREFIXES(JsonRpcServiceUnitTest, Reset);
  static bool IsValidDomain(const std::string& domain);
  static bool IsValidUnstoppableDomain(const std::string& domain);

  void OnGetERC721OwnerOf(GetERC721OwnerOfCallback callback,
                          APIRequestResult api_request_result);

  void OnGetSupportsInterfaceTokenMetadata(const std::string& contract_address,
                                           const std::string& signature,
                                           const GURL& network_url,
                                           GetTokenMetadataCallback callback,
                                           bool is_supported,
                                           mojom::ProviderError error,
                                           const std::string& error_message);

  void ContinueGetERC721TokenBalance(const std::string& account_address,
                                     GetERC721TokenBalanceCallback callback,
                                     const std::string& owner_address,
                                     mojom::ProviderError error,
                                     const std::string& error_message);
  void OnGetTokenUri(GetTokenMetadataCallback callback,
                     const APIRequestResult api_request_result);

  void OnGetTokenMetadataPayload(GetTokenMetadataCallback callback,
                                 APIRequestResult api_request_result);

  void OnSanitizeTokenMetadata(GetTokenMetadataCallback callback,
                               data_decoder::JsonSanitizer::Result result);

  void OnGetSupportsInterface(GetSupportsInterfaceCallback callback,
                              APIRequestResult api_request_result);

  // Solana
  void OnGetSolanaBalance(GetSolanaBalanceCallback callback,
                          APIRequestResult api_request_result);
  void OnGetSPLTokenAccountBalance(GetSPLTokenAccountBalanceCallback callback,

                                   APIRequestResult api_request_result);
  void OnSendSolanaTransaction(SendSolanaTransactionCallback callback,
                               APIRequestResult api_request_result);
  void OnGetSolanaLatestBlockhash(GetSolanaLatestBlockhashCallback callback,
                                  APIRequestResult api_request_result);
  void OnGetSolanaSignatureStatuses(GetSolanaSignatureStatusesCallback callback,
                                    APIRequestResult api_request_result);
  void OnGetSolanaAccountInfo(GetSolanaAccountInfoCallback callback,
                              APIRequestResult api_request_result);
  void OnGetSolanaFeeForMessage(GetSolanaFeeForMessageCallback callback,
                                APIRequestResult api_request_result);
  void OnGetSolanaBlockHeight(GetSolanaBlockHeightCallback callback,
                              APIRequestResult api_request_result);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  std::unique_ptr<APIRequestHelper> api_request_helper_;
  std::unique_ptr<APIRequestHelper> api_request_helper_ens_offchain_;
  base::flat_map<mojom::CoinType, GURL> network_urls_;
  // <mojom::CoinType, chain_id>
  base::flat_map<mojom::CoinType, std::string> chain_ids_;
  // <chain_id, mojom::AddChainRequest>
  base::flat_map<std::string, mojom::AddChainRequestPtr>
      add_chain_pending_requests_;
  // <origin, chain_id>
  base::flat_map<url::Origin, std::string> switch_chain_requests_;
  base::flat_map<url::Origin, RequestCallback> switch_chain_callbacks_;
  base::flat_map<url::Origin, base::Value> switch_chain_ids_;

  std::unique_ptr<unstoppable_domains::MultichainCalls<std::string>>
      ud_get_eth_addr_calls_;
  std::unique_ptr<unstoppable_domains::MultichainCalls<GURL>>
      ud_resolve_dns_calls_;

  mojo::RemoteSet<mojom::JsonRpcServiceObserver> observers_;

  EnsResolverTaskContainer<EnsGetEthAddrCallback> ens_get_eth_addr_tasks_;
  EnsResolverTaskContainer<EnsGetContentHashCallback>
      ens_get_content_hash_tasks_;

  mojo::ReceiverSet<mojom::JsonRpcService> receivers_;
  PrefService* prefs_ = nullptr;
  PrefService* local_state_prefs_ = nullptr;
  base::WeakPtrFactory<JsonRpcService> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_SERVICE_H_
