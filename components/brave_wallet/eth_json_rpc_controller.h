/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_ETH_JSON_RPC_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_ETH_JSON_RPC_CONTROLLER_H_

#include <list>
#include <map>
#include <memory>
#include <string>

#include "base/callback.h"
#include "brave/components/brave_wallet/brave_wallet_constants.h"
#include "url/gurl.h"

namespace content {
class BrowserContext;
}  // namespace content

namespace network {
class SimpleURLLoader;
}  // namespace network

namespace brave_wallet {

class EthJsonRpcController {
 public:
  EthJsonRpcController(content::BrowserContext* context, Network network);
  ~EthJsonRpcController();

  using URLRequestCallback =
      base::OnceCallback<void(const int,
                              const std::string&,
                              const std::map<std::string, std::string>&)>;
  void Request(const std::string& json_payload,
               URLRequestCallback callback,
               bool auto_retry_on_network_change);

  Network GetNetwork() const;
  GURL GetNetworkURL() const;
  void SetNetwork(Network network);
  void SetCustomNetwork(const GURL& provider_url);

 private:
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;
  void OnURLLoaderComplete(SimpleURLLoaderList::iterator iter,
                           URLRequestCallback callback,
                           const std::unique_ptr<std::string> response_body);

  content::BrowserContext* context_;
  GURL network_url_;
  SimpleURLLoaderList url_loaders_;
  Network network_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_ETH_JSON_RPC_CONTROLLER_H_
