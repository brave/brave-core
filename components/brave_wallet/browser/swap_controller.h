/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_CONTROLLER_H_
#define BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_CONTROLLER_H_

#include <map>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/components/api_request_helper/api_request_helper.h"
#include "brave/components/brave_wallet/browser/asset_ratio_response_parser.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace brave_wallet {

class SwapController {
 public:
  SwapController(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~SwapController();

  using GetPriceQuoteCallback =
      base::OnceCallback<void(bool status,
                              brave_wallet::mojom::SwapResponsePtr value)>;
  using GetTransactionPayloadCallback =
      base::OnceCallback<void(bool status,
                              brave_wallet::mojom::SwapResponsePtr value)>;
  // Obtians a quote for the specified asset
  void GetPriceQuote(const mojom::SwapParams& swap_params,
                     GetPriceQuoteCallback callback);
  // Obtains the transaction payload to be signed.
  void GetTransactionPayload(const mojom::SwapParams& swap_params,
                             GetTransactionPayloadCallback callback);

  static GURL GetPriceQuoteURL(const mojom::SwapParams& swap_params);
  static GURL GetTransactionPayloadURL(const mojom::SwapParams& swap_params);
  static void SetBaseURLForTest(const GURL& base_url_for_test);

 private:
  void OnGetPriceQuote(GetPriceQuoteCallback callback,
                       const int status,
                       const std::string& body,
                       const std::map<std::string, std::string>& headers);
  void OnGetTransactionPayload(
      GetTransactionPayloadCallback callback,
      const int status,
      const std::string& body,
      const std::map<std::string, std::string>& headers);

  static GURL base_url_for_test_;
  api_request_helper::APIRequestHelper api_request_helper_;
  base::WeakPtrFactory<SwapController> weak_ptr_factory_;
};

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_BRAVE_WALLET_BROWSER_SWAP_CONTROLLER_H_
