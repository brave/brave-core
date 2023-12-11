/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_RPC_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_RPC_H_

#include <list>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "base/types/expected.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/bitcoin_rpc_responses.h"
#include "components/prefs/pref_service.h"

namespace brave_wallet::bitcoin_rpc {

using UnspentOutputs = std::vector<UnspentOutput>;

struct QueuedRequestData;

class BitcoinRpc {
 public:
  explicit BitcoinRpc(
      PrefService* prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~BitcoinRpc();

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
  using GetFeeEstimatesCallback =
      RpcResponseCallback<std::map<uint32_t, double>>;
  using GetTransactionCallback = RpcResponseCallback<Transaction>;
  using GetAddressStatsCallback = RpcResponseCallback<AddressStats>;
  using GetUtxoListCallback = RpcResponseCallback<UnspentOutputs>;
  using PostTransactionCallback = RpcResponseCallback<std::string>;

  void GetChainHeight(const std::string& chain_id,
                      GetChainHeightCallback callback);
  void GetFeeEstimates(const std::string& chain_id,
                       GetFeeEstimatesCallback callback);
  void GetTransaction(const std::string& chain_id,
                      const std::string& txid,
                      GetTransactionCallback callback);
  void GetAddressStats(const std::string& chain_id,
                       const std::string& address,
                       GetAddressStatsCallback callback);
  void GetUtxoList(const std::string& chain_id,
                   const std::string& address,
                   GetUtxoListCallback callback);

  void PostTransaction(const std::string& chain_id,
                       const std::vector<uint8_t>& transaction,
                       PostTransactionCallback callback);

  void SetUrlLoaderFactoryForTesting(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

 private:
  void RequestInternal(const GURL& request_url,
                       RequestIntermediateCallback callback,
                       APIRequestHelper::ResponseConversionCallback
                           conversion_callback = base::NullCallback());
  void OnRequestInternalDone(RequestIntermediateCallback callback,
                             APIRequestResult api_request_result);
  void MaybeStartQueuedRequest();

  void OnGetChainHeight(GetChainHeightCallback callback,
                        APIRequestResult api_request_result);
  void OnGetFeeEstimates(GetFeeEstimatesCallback callback,
                         APIRequestResult api_request_result);
  void OnGetTransaction(GetTransactionCallback callback,
                        APIRequestResult api_request_result);
  void OnGetAddressStats(GetAddressStatsCallback callback,
                         APIRequestResult api_request_result);
  void OnGetUtxoList(GetUtxoListCallback callback,
                     const std::string& address,
                     APIRequestResult api_request_result);

  void OnPostTransaction(PostTransactionCallback callback,
                         APIRequestResult api_request_result);

  const raw_ptr<PrefService> prefs_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  uint32_t active_requests_ = 0;
  base::circular_deque<QueuedRequestData> requests_queue_;
  std::unique_ptr<APIRequestHelper> api_request_helper_;
  base::WeakPtrFactory<BitcoinRpc> weak_ptr_factory_{this};
};

}  // namespace brave_wallet::bitcoin_rpc

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_BITCOIN_BITCOIN_RPC_H_
