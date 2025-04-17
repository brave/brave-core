/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_RPC_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_RPC_H_

#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/cardano/cardano_rpc_responses.h"

namespace brave_wallet {
class NetworkManager;
}

namespace brave_wallet::cardano_rpc {

using UnspentOutputs = std::vector<UnspentOutput>;

struct EndpointQueue;

class CardanoRpc {
 public:
  CardanoRpc(NetworkManager& network_manager,
             scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~CardanoRpc();

  using APIRequestHelper = api_request_helper::APIRequestHelper;
  using APIRequestResult = api_request_helper::APIRequestResult;
  using RequestIntermediateCallback =
      base::OnceCallback<void(APIRequestResult api_request_result)>;
  using ResponseConversionCallback =
      api_request_helper::APIRequestHelper::ResponseConversionCallback;

  template <class T>
  using RpcResponseCallback =
      base::OnceCallback<void(base::expected<T, std::string>)>;

  using GetChainHeightCallback = RpcResponseCallback<uint32_t>;
  void GetChainHeight(const std::string& chain_id,
                      GetChainHeightCallback callback);

  using GetUtxoListCallback = RpcResponseCallback<UnspentOutputs>;
  void GetUtxoList(const std::string& chain_id,
                   const std::string& address,
                   GetUtxoListCallback callback);

  void SetUrlLoaderFactoryForTesting(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

 private:
  void RequestInternal(const GURL& request_url,
                       RequestIntermediateCallback callback,
                       APIRequestHelper::ResponseConversionCallback
                           conversion_callback = base::NullCallback());
  void OnRequestInternalDone(const GURL& endpoint_host,
                             RequestIntermediateCallback callback,
                             APIRequestResult api_request_result);
  void MaybeStartQueuedRequest(const GURL& endpoint_host);

  void OnGetChainHeight(GetChainHeightCallback callback,
                        APIRequestResult api_request_result);
  void OnGetUtxoList(GetUtxoListCallback callback,
                     const std::string& address,
                     APIRequestResult api_request_result);

  GURL GetNetworkURL(const std::string& chain_id);

  const raw_ref<NetworkManager> network_manager_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  // Uses hostname as key. Tracks request throttling(if required) per host.
  std::map<std::string, EndpointQueue> endpoints_;
  APIRequestHelper api_request_helper_;
  base::WeakPtrFactory<CardanoRpc> weak_ptr_factory_{this};
};

}  // namespace brave_wallet::cardano_rpc

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_CARDANO_CARDANO_RPC_H_
