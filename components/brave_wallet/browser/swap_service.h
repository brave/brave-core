/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_SERVICE_H_

#include <string>

#include "base/memory/weak_ptr.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave_wallet {

class SwapService : public KeyedService, public mojom::SwapService {
 public:
  using APIRequestResult = api_request_helper::APIRequestResult;

  explicit SwapService(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~SwapService() override;
  SwapService(const SwapService&) = delete;
  SwapService& operator=(const SwapService&) = delete;

  mojo::PendingRemote<mojom::SwapService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::SwapService> receiver);

  // Obtains a quote for a swap.
  void GetQuote(mojom::SwapQuoteParamsPtr params,
                GetQuoteCallback callback) override;
  void GetTransaction(mojom::SwapTransactionParamsUnionPtr params,
                      GetTransactionCallback callback) override;

  // Obtains whether the given chain_id supports swap.
  void IsSwapSupported(const std::string& chain_id,
                       IsSwapSupportedCallback callback) override;

  void GetLiFiStatus(const std::string& tx_hash,
                     GetLiFiStatusCallback callback) override;

  static GURL GetZeroExQuoteURL(const mojom::SwapQuoteParams& params,
                                const std::optional<std::string>& fee_param);
  static GURL GetZeroExTransactionURL(
      const mojom::SwapQuoteParams& params,
      const std::optional<std::string>& fee_param);
  static GURL GetJupiterQuoteURL(const mojom::SwapQuoteParams& params,
                                 const std::optional<std::string>& fee_param);
  static GURL GetJupiterTransactionURL(const std::string& chain_id);
  static GURL GetLiFiQuoteURL();
  static GURL GetLiFiTransactionURL();
  static GURL GetLiFiStatusURL(const std::string& tx_hash);
  static GURL GetSquidURL();

 private:
  void OnGetZeroExQuote(const std::string& chain_id,
                        mojom::SwapFeesPtr swap_fee,
                        GetQuoteCallback callback,
                        APIRequestResult api_request_result);
  void OnGetJupiterQuote(mojom::SwapFeesPtr swap_fee,
                         GetQuoteCallback callback,
                         APIRequestResult api_request_result);
  void OnGetLiFiQuote(mojom::SwapFeesPtr swap_fee,
                      GetQuoteCallback callback,
                      APIRequestResult api_request_result);
  void OnGetSquidQuote(mojom::SwapFeesPtr swap_fee,
                       GetQuoteCallback callback,
                       APIRequestResult api_request_result);
  void OnGetZeroExTransaction(GetTransactionCallback callback,
                              APIRequestResult api_request_result);
  void OnGetJupiterTransaction(GetTransactionCallback callback,
                               APIRequestResult api_request_result);
  void OnGetLiFiTransaction(GetTransactionCallback callback,
                            APIRequestResult api_request_result);
  void OnGetSquidTransaction(GetTransactionCallback callback,
                             APIRequestResult api_request_result);
  void OnGetLiFiStatus(GetLiFiStatusCallback callback,
                       APIRequestResult api_request_result);

  api_request_helper::APIRequestHelper api_request_helper_;

  mojo::ReceiverSet<mojom::SwapService> receivers_;

  base::WeakPtrFactory<SwapService> weak_ptr_factory_{this};
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_SERVICE_H_
