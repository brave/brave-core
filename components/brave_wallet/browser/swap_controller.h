/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_CONTROLLER_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/containers/flat_map.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/asset_ratio_response_parser.h"
#include "url/gurl.h"

#include "brave/components/brave_wallet/common/brave_wallet.mojom.h"
#include "components/keyed_service/core/keyed_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver_set.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave_wallet {

class SwapController : public KeyedService, public mojom::SwapController {
 public:
  SwapController(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~SwapController() override;
  SwapController(const SwapController&) = delete;
  SwapController& operator=(const SwapController&) = delete;

  mojo::PendingRemote<mojom::SwapController> MakeRemote();
  void Bind(mojo::PendingReceiver<mojom::SwapController> receiver);

  // Obtians a quote for the specified asset
  void GetPriceQuote(mojom::SwapParamsPtr swap_params,
                     GetPriceQuoteCallback callback) override;
  // Obtains the transaction payload to be signed.
  void GetTransactionPayload(mojom::SwapParamsPtr swap_params,
                             GetTransactionPayloadCallback callback) override;

  static GURL GetPriceQuoteURL(const mojom::SwapParamsPtr swap_params);
  static GURL GetTransactionPayloadURL(mojom::SwapParamsPtr swap_params);
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

  mojo::ReceiverSet<mojom::SwapController> receivers_;

  base::WeakPtrFactory<SwapController> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_CONTROLLER_H_
