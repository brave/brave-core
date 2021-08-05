/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_JSON_RPC_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_JSON_RPC_CONTROLLER_H_

#include <list>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list_threadsafe.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_types.h"
#include "brave/components/brave_wallet/browser/eth_json_rpc_controller_events_observer.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
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

namespace brave_wallet {

class EthJsonRpcController : public KeyedService,
                             public mojom::EthJsonRpcController {
 public:
  EthJsonRpcController(
      mojom::Network network,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~EthJsonRpcController() override;

  mojo::PendingRemote<mojom::EthJsonRpcController> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::EthJsonRpcController> receiver);

  using GetBlockNumberCallback =
      base::OnceCallback<void(bool status, uint256_t result)>;
  void GetBlockNumber(GetBlockNumberCallback callback);

  void Request(const std::string& json_payload,
               bool auto_retry_on_network_change,
               RequestCallback callback) override;
  void GetBalance(const std::string& address,
                  GetBalanceCallback callback) override;

  void GetTransactionCount(const std::string& address,
                           GetTransactionCountCallback callback) override;

  void GetTransactionReceipt(const std::string& tx_hash,
                             GetTransactionReceiptCallback callback) override;

  void SendRawTransaction(const std::string& signed_tx,
                          SendRawTransactionCallback callback) override;

  void GetERC20TokenBalance(const std::string& conract_address,
                            const std::string& address,
                            GetERC20TokenBalanceCallback callback) override;

  // Call getMany function of ProxyReader contract from Unstoppable Domains.
  void UnstoppableDomainsProxyReaderGetMany(
      const std::string& contract_address,
      const std::string& domain,
      const std::vector<std::string>& keys,
      UnstoppableDomainsProxyReaderGetManyCallback callback) override;

  void EnsProxyReaderGetResolverAddress(
      const std::string& contract_address,
      const std::string& domain,
      UnstoppableDomainsProxyReaderGetManyCallback callback) override;

  bool EnsProxyReaderResolveAddress(
      const std::string& contract_address,
      const std::string& domain,
      UnstoppableDomainsProxyReaderGetManyCallback callback);

  void GetNetwork(
      mojom::EthJsonRpcController::GetNetworkCallback callback) override;
  void SetNetwork(mojom::Network network) override;
  void GetChainId(
      mojom::EthJsonRpcController::GetChainIdCallback callback) override;
  void GetBlockTrackerUrl(
      mojom::EthJsonRpcController::GetBlockTrackerUrlCallback callback)
      override;
  void GetNetworkUrl(
      mojom::EthJsonRpcController::GetNetworkUrlCallback callback) override;
  void SetCustomNetwork(const GURL& provider_url) override;

  void AddObserver(::mojo::PendingRemote<mojom::EthJsonRpcControllerObserver>
                       observer) override;

  // Returns the chain ID for a network or an empty string if no standard
  // chain ID is defined for the specified network.
  static std::string GetChainIdFromNetwork(mojom::Network network);
  static GURL GetBlockTrackerUrlFromNetwork(mojom::Network network);

 private:
  void FireNetworkChanged();
  void OnGetBlockNumber(
      GetBlockNumberCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);
  void OnGetBalance(GetBalanceCallback callback,
                    const int status,
                    const std::string& body,
                    const base::flat_map<std::string, std::string>& headers);
  void OnGetTransactionCount(
      GetTransactionCountCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);
  void OnGetTransactionReceipt(
      GetTransactionReceiptCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);
  void OnSendRawTransaction(
      SendRawTransactionCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);
  void OnGetERC20TokenBalance(
      GetERC20TokenBalanceCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  void OnUnstoppableDomainsProxyReaderGetMany(
      UnstoppableDomainsProxyReaderGetManyCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  void OnEnsProxyReaderGetResolverAddress(
      UnstoppableDomainsProxyReaderGetManyCallback callback,
      const std::string& domain,
      int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  void OnEnsProxyReaderResolveAddress(
      UnstoppableDomainsProxyReaderGetManyCallback callback,
      int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  api_request_helper::APIRequestHelper api_request_helper_;
  GURL network_url_;
  mojom::Network network_;
  mojo::RemoteSet<mojom::EthJsonRpcControllerObserver> observers_;

  mojo::ReceiverSet<mojom::EthJsonRpcController> receivers_;

  base::WeakPtrFactory<EthJsonRpcController> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_JSON_RPC_CONTROLLER_H_
