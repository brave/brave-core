/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_ORDINALS_RPC_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_ORDINALS_RPC_H_

#include <map>
#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/bitcoin_ordinals_rpc_responses.h"
#include "brave/components/brave_wallet/common/bitcoin_utils.h"

namespace brave_wallet::bitcoin_ordinals_rpc {

struct EndpointQueue;

class BitcoinOrdinalsRpc {
 public:
  explicit BitcoinOrdinalsRpc(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~BitcoinOrdinalsRpc();

  using APIRequestHelper = api_request_helper::APIRequestHelper;
  using APIRequestResult = api_request_helper::APIRequestResult;
  using RequestIntermediateCallback =
      base::OnceCallback<void(APIRequestResult api_request_result)>;
  using ResponseConversionCallback =
      api_request_helper::APIRequestHelper::ResponseConversionCallback;

  template <class T>
  using RpcResponseCallback =
      base::OnceCallback<void(base::expected<T, std::string>)>;

  using GetOutpointInfoCallback = RpcResponseCallback<OutpointInfo>;
  void GetOutpointInfo(const std::string& chain_id,
                       const BitcoinOutpoint& outpoint,
                       GetOutpointInfoCallback callback);

  void SetUrlLoaderFactoryForTesting(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

 private:
  void RequestInternal(const GURL& request_url,
                       RequestIntermediateCallback callback,
                       APIRequestHelper::ResponseConversionCallback
                           conversion_callback = base::NullCallback());
  void OnRequestInternalDone(const std::string& endpoint_host,
                             RequestIntermediateCallback callback,
                             APIRequestResult api_request_result);
  void MaybeStartQueuedRequest(const std::string& endpoint_host);

  void OnGetOutpointInfo(GetOutpointInfoCallback callback,
                         APIRequestResult api_request_result);

  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  // Uses hostname as key. Tracks request throttling(if required) per host.
  std::map<std::string, EndpointQueue> endpoints_;
  std::unique_ptr<APIRequestHelper> api_request_helper_;
  base::WeakPtrFactory<BitcoinOrdinalsRpc> weak_ptr_factory_{this};
};

}  // namespace brave_wallet::bitcoin_ordinals_rpc

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_ORDINALS_RPC_H_
