/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_NETWORK_CLIENT_OBLIVIOUS_HTTP_CLIENT_IMPL_H_
#define BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_NETWORK_CLIENT_OBLIVIOUS_HTTP_CLIENT_IMPL_H_

#include "brave/components/search_query_metrics/network_client/network_client_callback.h"
#include "services/network/public/mojom/oblivious_http_request.mojom.h"

class GURL;

namespace metrics {

class ObliviousHttpClientImpl : public network::mojom::ObliviousHttpClient {
 public:
  ObliviousHttpClientImpl(GURL url, SendRequestCallback callback);

  ObliviousHttpClientImpl(const ObliviousHttpClientImpl&) = delete;
  ObliviousHttpClientImpl& operator=(const ObliviousHttpClientImpl&) = delete;

  ~ObliviousHttpClientImpl() override;

  void OnCompleted(
      network::mojom::ObliviousHttpCompletionResultPtr response) override;

 private:
  const GURL url_;
  SendRequestCallback callback_;
};

}  // namespace metrics

#endif  // BRAVE_COMPONENTS_SEARCH_QUERY_METRICS_NETWORK_CLIENT_OBLIVIOUS_HTTP_CLIENT_IMPL_H_
