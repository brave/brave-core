/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_H_

#include <memory>
#include <optional>
#include <string>

#include "base/memory/raw_ref.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/browser/service/network_client_callback.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "services/network/public/cpp/network_context_getter.h"
#include "url/gurl.h"

class PrefService;

namespace network {
class SimpleURLLoader;
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_ads {

class ObliviousHttpKeyConfig;

// Sends network requests, supporting both standard HTTP and Oblivious HTTP
// (OHTTP). Standard HTTP requests are issued via SimpleURLLoader, while OHTTP
// requests are routed through the network service’s OHTTP implementation.
class NetworkClient {
 public:
  NetworkClient(
      PrefService& local_state,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      network::NetworkContextGetter network_context_getter,
      bool use_ohttp_staging);

  NetworkClient(const NetworkClient&) = delete;
  NetworkClient& operator=(const NetworkClient&) = delete;

  ~NetworkClient();

  // Issues a network request described by `mojom::UrlRequestInfo`. When the
  // request completes, `callback` is invoked with a `mojom::UrlResponseInfo`.
  // The callback will not run if the request is canceled or if this instance
  // is destroyed.
  void SendRequest(mojom::UrlRequestInfoPtr mojom_url_request,
                   SendRequestCallback callback);

  // Cancels all active requests. Any pending callbacks will be dropped.
  void CancelRequests();

 private:
  // Sends the request using standard HTTP.
  void HttpRequest(mojom::UrlRequestInfoPtr mojom_url_request,
                   SendRequestCallback callback);
  void HttpRequestCallback(std::unique_ptr<network::SimpleURLLoader> url_loader,
                           SendRequestCallback callback,
                           std::optional<std::string> response_body);

  // Sends the request using Oblivious HTTP (OHTTP). For details, see
  // https://ietf-wg-ohai.github.io/oblivious-http/draft-ietf-ohai-ohttp.html
  void ObliviousHttpRequest(mojom::UrlRequestInfoPtr mojom_url_request,
                            const GURL& relay_url,
                            SendRequestCallback callback);
  void ObliviousHttpRequestCallback(
      SendRequestCallback callback,
      mojom::UrlResponseInfoPtr mojom_url_response);

  const raw_ref<PrefService> local_state_;

  const scoped_refptr<network::SharedURLLoaderFactory>
      url_loader_factory_;  // Not owned.

  const network::NetworkContextGetter network_context_getter_;

  const std::unique_ptr<ObliviousHttpKeyConfig> oblivious_http_key_config_;
  const GURL oblivious_http_relay_url_;

  base::WeakPtrFactory<NetworkClient> weak_ptr_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_H_
