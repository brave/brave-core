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
#include "base/observer_list_threadsafe.h"
#include "brave/components/brave_wallet/browser/brave_wallet_constants.h"
#include "brave/components/brave_wallet/browser/brave_wallet_provider_events_observer.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave_wallet {

class EthJsonRpcController {
 public:
  EthJsonRpcController(
      Network network,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~EthJsonRpcController();

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

  using GetERC20TokenBalanceCallback =
      base::OnceCallback<void(bool status, const std::string& balance)>;
  bool GetERC20TokenBalance(const std::string& conract_address,
                            const std::string& address,
                            GetERC20TokenBalanceCallback callback);

  // Call getMany function of ProxyReader contract from Unstoppable Domains.
  using UnstoppableDomainsProxyReaderGetManyCallback =
      base::OnceCallback<void(bool status, const std::string& result)>;
  bool UnstoppableDomainsProxyReaderGetMany(
      const std::string& contract_address,
      const std::string& domain,
      const std::vector<std::string>& keys,
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
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;
  void OnURLLoaderComplete(SimpleURLLoaderList::iterator iter,
                           URLRequestCallback callback,
                           const std::unique_ptr<std::string> response_body);
  void OnGetBalance(GetBallanceCallback callback,
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

  GURL network_url_;
  SimpleURLLoaderList url_loaders_;
  Network network_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  scoped_refptr<base::ObserverListThreadSafe<BraveWalletProviderEventsObserver>>
      observers_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_ETH_JSON_RPC_CONTROLLER_H_
