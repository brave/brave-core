/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_SERVICE_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/asset_ratio_response_parser.h"
#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave_wallet {

class JsonRpcService;

class SwapService : public KeyedService, public mojom::SwapService {
 public:
  SwapService(scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
              JsonRpcService* json_rpc_service);
  ~SwapService() override;
  SwapService(const SwapService&) = delete;
  SwapService& operator=(const SwapService&) = delete;

  mojo::PendingRemote<mojom::SwapService> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::SwapService> receiver);

  // Obtians a quote for the specified asset
  void GetPriceQuote(mojom::SwapParamsPtr swap_params,
                     GetPriceQuoteCallback callback) override;
  // Obtains the transaction payload to be signed.
  void GetTransactionPayload(mojom::SwapParamsPtr swap_params,
                             GetTransactionPayloadCallback callback) override;

  static std::string GetFee(const std::string& chain_id);
  static std::string GetBaseSwapURL(const std::string& chain_id);
  static std::string GetFeeRecipient(const std::string& chain_id);
  static std::string GetAffiliateAddress(const std::string& chain_id);

  static GURL GetPriceQuoteURL(const mojom::SwapParamsPtr swap_params,
                               const std::string& chain_id);
  static GURL GetTransactionPayloadURL(mojom::SwapParamsPtr swap_params,
                                       const std::string& chain_id);
  static void SetBaseURLForTest(const GURL& base_url_for_test);

 private:
  void OnGetPriceQuote(GetPriceQuoteCallback callback,
                       const int status,
                       const std::string& body,
                       const base::flat_map<std::string, std::string>& headers);
  void OnGetTransactionPayload(
      GetTransactionPayloadCallback callback,
      const int status,
      const std::string& body,
      const base::flat_map<std::string, std::string>& headers);

  static GURL base_url_for_test_;
  api_request_helper::APIRequestHelper api_request_helper_;

  raw_ptr<JsonRpcService> json_rpc_service_ = nullptr;  // NOT OWNED
  mojo::ReceiverSet<mojom::SwapService> receivers_;

  base::WeakPtrFactory<SwapService> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_SERVICE_H_
