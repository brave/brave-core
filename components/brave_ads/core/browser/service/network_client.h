/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_H_

#include <memory>
#include <optional>
#include <string>

#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/browser/service/network_client_callback.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "services/network/public/cpp/network_context_getter.h"

namespace network {
class SimpleURLLoader;
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_ads {

// Sends network requests, supporting standard HTTP. Standard HTTP requests are
// issued via `SimpleURLLoader`.
class NetworkClient {
 public:
  NetworkClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      network::NetworkContextGetter network_context_getter);

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

  const scoped_refptr<network::SharedURLLoaderFactory>
      url_loader_factory_;  // Not owned.

  const network::NetworkContextGetter network_context_getter_;

  base::WeakPtrFactory<NetworkClient> weak_ptr_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_H_
