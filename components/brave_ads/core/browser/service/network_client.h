/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_H_

#include <memory>
#include <optional>
#include <string>

#include "base/containers/flat_set.h"
#include "base/containers/unique_ptr_adapters.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/public/network_client_callback.h"
#include "services/network/public/mojom/network_context.mojom-forward.h"

class GURL;

namespace network {
class SimpleURLLoader;
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_ads {

class NetworkClientObliviousHttpKeyConfig;

// This class is responsible for sending HTTP and Oblivious HTTP network
// requests.
class NetworkClient {
 public:
  NetworkClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      network::mojom::NetworkContext* mojom_network_context,
      bool use_staging);

  NetworkClient(const NetworkClient&) = delete;
  NetworkClient& operator=(const NetworkClient&) = delete;

  ~NetworkClient();

  // Starts a network request for the given `mojom::UrlRequestInfo`. The
  // provided `callback` will be invoked with a `mojom::UrlResponseInfo` unless
  // the request is canceled or the `NetworkClient` is destroyed.
  void UrlRequest(mojom::UrlRequestInfoPtr mojom_url_request,
                  UrlRequestCallback callback);

  // Cancels all ongoing network requests. Pending callbacks will not be
  // invoked.
  void CancelRequests();

 private:
  // Uses standard HTTP to perform the request.
  void HttpRequest(mojom::UrlRequestInfoPtr mojom_url_request,
                   UrlRequestCallback callback);
  void HttpRequestCallback(network::SimpleURLLoader* url_loader,
                           UrlRequestCallback callback,
                           std::optional<std::string> response_body);

  // Uses Oblivous HTTP indirectly to perform the request. See specification at
  // https://ietf-wg-ohai.github.io/oblivious-http/draft-ietf-ohai-ohttp.html
  void FetchObliviousHttpKeyConfig();
  void ObliviousHttpRequest(mojom::UrlRequestInfoPtr mojom_url_request,
                            const GURL& relay_url,
                            UrlRequestCallback callback);

  const scoped_refptr<network::SharedURLLoaderFactory>
      url_loader_factory_;  // Not owned.
  base::flat_set<std::unique_ptr<network::SimpleURLLoader>,
                 base::UniquePtrComparator>
      url_loaders_;

  const raw_ptr<network::mojom::NetworkContext>
      mojom_network_context_;  // Not owned.

  const bool use_staging_ = false;

  std::unique_ptr<NetworkClientObliviousHttpKeyConfig>
      oblivious_http_key_config_;

  base::WeakPtrFactory<NetworkClient> weak_ptr_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_H_
