/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_OBLIVIOUS_HTTP_IMPL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_OBLIVIOUS_HTTP_IMPL_H_

#include "brave/components/brave_ads/core/public/network_client_callback.h"
#include "services/network/public/mojom/oblivious_http_request.mojom.h"

class GURL;

namespace brave_ads {

class ObliviousHttpClientImpl : public network::mojom::ObliviousHttpClient {
 public:
  ObliviousHttpClientImpl(GURL url, UrlRequestCallback callback);

  ObliviousHttpClientImpl(const ObliviousHttpClientImpl&) = delete;
  ObliviousHttpClientImpl& operator=(const ObliviousHttpClientImpl&) = delete;

  ~ObliviousHttpClientImpl() override;

  void OnCompleted(network::mojom::ObliviousHttpCompletionResultPtr
                       mojom_http_completion_result) override;

 private:
  GURL url_;
  UrlRequestCallback url_request_callback_;
};
}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_OBLIVIOUS_HTTP_IMPL_H_
