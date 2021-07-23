/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_JSON_RPC_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_JSON_RPC_CONTROLLER_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/observer_list_threadsafe.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_events_observer.h"
#include "brave/components/brave_wallet/browser/brave_wallet_types.h"
#include "components/keyed_service/core/keyed_service.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave_wallet {

class EthJsonRpcController : public KeyedService {
 public:
  EthJsonRpcController(
      Network network,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~EthJsonRpcController() override;

  using URLRequestCallback =
      base::OnceCallback<void(const int,
                              const std::string&,
                              const std::map<std::string, std::string>&)>;
  void Request(const std::string& json_payload,
               URLRequestCallback callback,
               bool auto_retry_on_network_change);
  using GetBallanceCallback =
      base::OnceCallback<void(bool status, const std::string& balance)>;
  void GetBalance(const std::string& address, GetBallanceCallback callback);

  using GetTxCountCallback =
      base::OnceCallback<void(bool status, uint256_t result)>;
  void GetTransactionCount(const std::string& address,
                           GetTxCountCallback callback);

  using GetTxReceiptCallback =
      base::OnceCallback<void(bool status, TransactionReceipt result)>;
  void GetTransactionReceipt(const std::string& tx_hash,
                             GetTxReceiptCallback callback);

  using SendRawTxCallback =
      base::OnceCallback<void(bool status, const std::string& tx_hash)>;
  void SendRawTransaction(const std::string& signed_tx,
                          SendRawTxCallback callback);

  using GetERC20TokenBalanceCallback =
      base::OnceCallback<void(bool status, const std::string& balance)>;
  bool GetERC20TokenBalance(const std::string& conract_address,
                            const std::string& address,
                            GetERC20TokenBalanceCallback callback);

  using UnstoppableDomainsProxyReaderGetManyCallback =
      base::OnceCallback<void(bool status, const std::string& result)>;

  // Call getMany function of ProxyReader contract from Unstoppable Domains.
  void UnstoppableDomainsProxyReaderGetMany(
      const std::string& contract_address,
      const std::string& domain,
      const std::vector<std::string>& keys,
      UnstoppableDomainsProxyReaderGetManyCallback callback);

  void EnsProxyReaderGetResolverAddress(
      const std::string& contract_address,
      const std::string& domain,
      UnstoppableDomainsProxyReaderGetManyCallback callback);

  bool EnsProxyReaderResolveAddress(
      const std::string& contract_address,
      const std::string& domain,
      UnstoppableDomainsProxyReaderGetManyCallback callback);

  Network GetNetwork() const;
  GURL GetNetworkURL() const;
  void SetNetwork(Network network);
  void SetCustomNetwork(const GURL& provider_url);

  void AddObserver(BraveWalletProviderEventsObserver* observer);
  void RemoveObserver(BraveWalletProviderEventsObserver* observer);

  // Returns the chain ID for a network or an empty string if no standard
  // chain ID is defined for the specified network.
  static std::string GetChainIDFromNetwork(Network network);
  static GURL GetBlockTrackerURLFromNetwork(Network network);

 private:
  void OnGetBalance(GetBallanceCallback callback,
                    const int status,
                    const std::string& body,
                    const std::map<std::string, std::string>& headers);
  void OnGetTransactionCount(GetTxCountCallback callback,
                             const int status,
                             const std::string& body,
                             const std::map<std::string, std::string>& headers);
  void OnGetTransactionReceipt(
      GetTxReceiptCallback callback,
      const int status,
      const std::string& body,
      const std::map<std::string, std::string>& headers);
  void OnSendRawTransaction(SendRawTxCallback callback,
                            const int status,
                            const std::string& body,
                            const std::map<std::string, std::string>& headers);
  void OnGetERC20TokenBalance(
      GetERC20TokenBalanceCallback callback,
      const int status,
      const std::string& body,
      const std::map<std::string, std::string>& headers);

  void OnUnstoppableDomainsProxyReaderGetMany(
      UnstoppableDomainsProxyReaderGetManyCallback callback,
      const int status,
      const std::string& body,
      const std::map<std::string, std::string>& headers);

  void OnEnsProxyReaderGetResolverAddress(
      UnstoppableDomainsProxyReaderGetManyCallback callback,
      const std::string& domain,
      int status,
      const std::string& body,
      const std::map<std::string, std::string>& headers);

  void OnEnsProxyReaderResolveAddress(
      UnstoppableDomainsProxyReaderGetManyCallback callback,
      int status,
      const std::string& body,
      const std::map<std::string, std::string>& headers);

  api_request_helper::APIRequestHelper api_request_helper_;
  GURL network_url_;
  Network network_;
  scoped_refptr<base::ObserverListThreadSafe<BraveWalletProviderEventsObserver>>
      observers_;

  base::WeakPtrFactory<EthJsonRpcController> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_JSON_RPC_CONTROLLER_H_
