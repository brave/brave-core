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
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "brave/components/brave_wallet/common/brave_wallet_types.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "mojo/public/cpp/bindings/remote_set.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

class PrefService;

namespace brave_wallet {

class JsonRpcService : public KeyedService, public mojom::JsonRpcService {
 public:
  JsonRpcService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* prefs);
  ~JsonRpcService() override;

  static void MigrateMultichainNetworks(PrefService* prefs);

  mojo::PendingRemote<mojom::JsonRpcService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::JsonRpcService> receiver);

  using StringResultCallback =
      base::OnceCallback<void(const std::string& result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;

  using GetBlockNumberCallback =
      base::OnceCallback<void(uint256_t result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  using RequestIntermediateCallback = base::OnceCallback<void(
      int http_code,
      const std::string& response,
      const base::flat_map<std::string, std::string>& headers)>;
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
                       const int code,
                       const std::string& message,
                       const base::flat_map<std::string, std::string>& headers);

  void GetBalance(const std::string& address,
                  mojom::CoinType coin,
                  const std::string& chaind_id,
                  GetBalanceCallback callback) override;

  using GetTxCountCallback =
      base::OnceCallback<void(uint256_t result,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  void GetTransactionCount(const std::string& address,
                           GetTxCountCallback callback);

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

  using UnstoppableDomainsProxyReaderGetManyCallback =
      base::OnceCallback<void(const std::vector<std::string>& values,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  // Call getMany function of ProxyReader contract from Unstoppable Domains.
  void UnstoppableDomainsProxyReaderGetMany(
      const std::string& chain_id,
      const std::string& domain,
      const std::vector<std::string>& keys,
      UnstoppableDomainsProxyReaderGetManyCallback callback);

  void UnstoppableDomainsGetEthAddr(
      const std::string& domain,
      UnstoppableDomainsGetEthAddrCallback callback) override;

  void EnsResolverGetContentHash(const std::string& chain_id,
                                 const std::string& domain,
                                 StringResultCallback callback);
  void EnsGetEthAddr(const std::string& domain,
                     EnsGetEthAddrCallback callback) override;

  bool SetNetwork(const std::string& chain_id, mojom::CoinType coin);
  void SetNetwork(const std::string& chain_id,
                  mojom::CoinType coin,
                  SetNetworkCallback callback) override;
  void GetNetwork(mojom::CoinType coin, GetNetworkCallback callback) override;
  void AddEthereumChain(mojom::NetworkInfoPtr chain,
                        AddEthereumChainCallback callback) override;
  void AddEthereumChainForOrigin(
      mojom::NetworkInfoPtr chain,
      const GURL& origin,
      AddEthereumChainForOriginCallback callback) override;
  void AddEthereumChainRequestCompleted(const std::string& chain_id,
                                        bool approved) override;
  void RemoveEthereumChain(const std::string& chain_id,
                           RemoveEthereumChainCallback callback) override;

  std::string GetChainId(mojom::CoinType coin) const;
  void GetChainId(mojom::CoinType coin,
                  mojom::JsonRpcService::GetChainIdCallback callback) override;
  void GetBlockTrackerUrl(
      mojom::JsonRpcService::GetBlockTrackerUrlCallback callback) override;
  void GetPendingChainRequests(
      GetPendingChainRequestsCallback callback) override;
  void GetPendingSwitchChainRequests(
      GetPendingSwitchChainRequestsCallback callback) override;
  void NotifySwitchChainRequestProcessed(bool approved,
                                         const GURL& origin) override;
  void GetAllNetworks(mojom::CoinType coin,
                      GetAllNetworksCallback callback) override;
  std::string GetNetworkUrl(mojom::CoinType coin) const;
  void GetNetworkUrl(
      mojom::CoinType coin,
      mojom::JsonRpcService::GetNetworkUrlCallback callback) override;
  void SetCustomNetworkForTesting(const std::string& chain_id,
                                  mojom::CoinType coin,
                                  const GURL& provider_url) override;

  void AddObserver(
      ::mojo::PendingRemote<mojom::JsonRpcServiceObserver> observer) override;

  GURL GetBlockTrackerUrlFromNetwork(std::string chain_id);

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
                        GetERC721OwnerOfCallback callback) override;

  void GetERC721TokenBalance(const std::string& contract_address,
                             const std::string& token_id,
                             const std::string& account_address,
                             const std::string& chain_id,
                             GetERC721TokenBalanceCallback callback) override;

  // Resets things back to the original state of BraveWalletService.
  // To be used when the Wallet is reset / erased
  void Reset();

  using GetSupportsInterfaceCallback =
      base::OnceCallback<void(bool is_supported,
                              mojom::ProviderError error,
                              const std::string& error_message)>;
  void GetSupportsInterface(const std::string& contract_address,
                            const std::string& interface_id,
                            GetSupportsInterfaceCallback callback);

  using SwitchEthereumChainRequestCallback =
      base::OnceCallback<void(mojom::ProviderError error,
                              const std::string& error_message)>;
  // return false when there is an error before processing request
  bool AddSwitchEthereumChainRequest(const std::string& chain_id,
                                     const GURL& origin,
                                     RequestCallback callback,
                                     base::Value id);

  void SetAPIRequestHelperForTesting(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

  // Solana JSON RPCs
  void GetSolanaBalance(const std::string& pubkey,
                        GetSolanaBalanceCallback callback) override;
  void GetSPLTokenAccountBalance(
      const std::string& pubkey,
      GetSPLTokenAccountBalanceCallback callback) override;
  using SendSolanaTransactionCallback =
      base::OnceCallback<void(const std::string& tx_hash,
                              mojom::SolanaProviderError error,
                              const std::string& error_message)>;
  void SendSolanaTransaction(const std::string& signed_tx,
                             SendSolanaTransactionCallback callback);
  using GetSolanaLatestBlockhashCallback =
      base::OnceCallback<void(const std::string& latest_blockhash,
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

 private:
  void FireNetworkChanged(mojom::CoinType coin);
  void FirePendingRequestCompleted(const std::string& chain_id,
                                   const std::string& error);
  bool HasRequestFromOrigin(const GURL& origin) const;
  void RemoveChainIdRequest(const std::string& chain_id);
  void OnGetBlockNumber(
      GetBlockNumberCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);
  void OnGetFeeHistory(GetFeeHistoryCallback callback,
                       const int status,
                       const std::string& body,
                       const base::flat_map<std::string, std::string>& headers);
  void OnEthGetBalance(GetBalanceCallback callback,
                       const int status,
                       const std::string& body,
                       const base::flat_map<std::string, std::string>& headers);
  void OnFilGetBalance(GetBalanceCallback callback,
                       const int status,
                       const std::string& body,
                       const base::flat_map<std::string, std::string>& headers);

  void OnGetTransactionCount(
      GetTxCountCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);
  void OnGetTransactionReceipt(
      GetTxReceiptCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);
  void OnSendRawTransaction(
      SendRawTxCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);
  void OnGetERC20TokenBalance(
      GetERC20TokenBalanceCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);
  void OnGetERC20TokenAllowance(
      GetERC20TokenAllowanceCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  void OnUnstoppableDomainsProxyReaderGetMany(
      UnstoppableDomainsProxyReaderGetManyCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  void OnUnstoppableDomainsGetEthAddr(
      UnstoppableDomainsGetEthAddrCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  void EnsRegistryGetResolver(const std::string& chain_id,
                              const std::string& domain,
                              StringResultCallback callback);

  void OnEnsRegistryGetResolver(
      StringResultCallback callback,
      int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  void ContinueEnsResolverGetContentHash(const std::string& chain_id,
                                         const std::string& domain,
                                         StringResultCallback callback,
                                         const std::string& resolver_address,
                                         mojom::ProviderError error,
                                         const std::string& error_message);

  void OnEnsResolverGetContentHash(
      StringResultCallback callback,
      int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  void ContinueEnsGetEthAddr(const std::string& domain,
                             StringResultCallback callback,
                             const std::string& resolver_address,
                             mojom::ProviderError error,
                             const std::string& error_message);

  void OnEnsGetEthAddr(StringResultCallback callback,
                       int status,
                       const std::string& body,
                       const base::flat_map<std::string, std::string>& headers);

  void OnGetEstimateGas(
      GetEstimateGasCallback callback,
      int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  void OnGetGasPrice(GetGasPriceCallback callback,
                     int status,
                     const std::string& body,
                     const base::flat_map<std::string, std::string>& headers);

  void OnGetIsEip1559(GetIsEip1559Callback callback,
                      int status,
                      const std::string& body,
                      const base::flat_map<std::string, std::string>& headers);

  void MaybeUpdateIsEip1559(const std::string& chain_id);
  void UpdateIsEip1559(const std::string& chain_id,
                       bool is_eip1559,
                       mojom::ProviderError error,
                       const std::string& error_message);

  void RequestInternal(const std::string& json_payload,
                       bool auto_retry_on_network_change,
                       const GURL& network_url,
                       RequestIntermediateCallback callback);
  void OnEthChainIdValidatedForOrigin(
      mojom::NetworkInfoPtr chain,
      const GURL& origin,
      AddEthereumChainForOriginCallback callback,
      bool success);

  void OnEthChainIdValidated(mojom::NetworkInfoPtr chain,
                             AddEthereumChainCallback callback,
                             bool success);

  FRIEND_TEST_ALL_PREFIXES(JsonRpcServiceUnitTest, IsValidDomain);
  FRIEND_TEST_ALL_PREFIXES(JsonRpcServiceUnitTest, Reset);
  bool IsValidDomain(const std::string& domain);

  void OnGetERC721OwnerOf(
      GetERC721OwnerOfCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  void ContinueGetERC721TokenBalance(const std::string& account_address,
                                     GetERC721TokenBalanceCallback callback,
                                     const std::string& owner_address,
                                     mojom::ProviderError error,
                                     const std::string& error_message);

  void OnGetSupportsInterface(
      GetSupportsInterfaceCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  // Solana
  void OnGetSolanaBalance(
      GetSolanaBalanceCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);
  void OnGetSPLTokenAccountBalance(
      GetSPLTokenAccountBalanceCallback callback,

      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);
  void OnSendSolanaTransaction(
      SendSolanaTransactionCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);
  void OnGetSolanaLatestBlockhash(
      GetSolanaLatestBlockhashCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);
  void OnGetSolanaSignatureStatuses(
      GetSolanaSignatureStatusesCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  std::unique_ptr<api_request_helper::APIRequestHelper> api_request_helper_;
  base::flat_map<mojom::CoinType, GURL> network_urls_;
  // <mojom::CoinType, chain_id>
  base::flat_map<mojom::CoinType, std::string> chain_ids_;
  // mojom::NetworkInfoPtr is move-only so we cannot use a custom struct to
  // store NetworkInfo and origin together
  // <chain_id, mojom::NetworkInfoPtr>
  base::flat_map<std::string, mojom::NetworkInfoPtr>
      add_chain_pending_requests_;
  // <chain_id, origin>
  base::flat_map<std::string, GURL> add_chain_pending_requests_origins_;
  // <origin, chain_id>
  base::flat_map<GURL, std::string> switch_chain_requests_;
  base::flat_map<GURL, RequestCallback> switch_chain_callbacks_;
  base::flat_map<GURL, base::Value> switch_chain_ids_;
  mojo::RemoteSet<mojom::JsonRpcServiceObserver> observers_;

  mojo::ReceiverSet<mojom::JsonRpcService> receivers_;
  PrefService* prefs_ = nullptr;
  base::WeakPtrFactory<JsonRpcService> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_JSON_RPC_SERVICE_H_
