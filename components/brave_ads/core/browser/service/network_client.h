/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_H_

#include <list>
#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/browser/service/network_client_callback.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"
#include "services/network/public/mojom/network_context.mojom-forward.h"

class GURL;

namespace network {
class SimpleURLLoader;
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_ads {

class NetworkClient {
 public:
  // Constructs a NetworkClient. If `use_staging_relay` is `true`, the staging
  // OHTTP relay server will be used for Oblivious HTTP requests; otherwise, the
  // production relay server will be used.
  NetworkClient(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      network::mojom::NetworkContext* mojom_network_context,
      bool use_staging_relay);

  NetworkClient(const NetworkClient&) = delete;
  NetworkClient& operator=(const NetworkClient&) = delete;

  ~NetworkClient();

  // Performs a URL request. If `mojom::UrlRequestInfo::use_ohttp` is `true` and
  // OHTTP support is enabled via Griffin, the request is made using Oblivious
  // HTTP; otherwise, a standard HTTP request is made.
  void UrlRequest(mojom::UrlRequestInfoPtr mojom_url_request,
                  UrlRequestCallback callback);

  // Cancels all ongoing requests.
  void CancelRequests();

 private:
  using URLLoaderList = std::list<std::unique_ptr<network::SimpleURLLoader>>;

  // Performs a standard HTTP request.
  void HttpRequest(mojom::UrlRequestInfoPtr mojom_url_request,
                   UrlRequestCallback callback);
  void HttpRequestCallback(URLLoaderList::iterator url_loaders_iter,
                           UrlRequestCallback callback,
                           std::unique_ptr<std::string> response_body);

  // Performs an Oblivious HTTP request.
  void ObliviousHttpRequest(mojom::UrlRequestInfoPtr mojom_url_request,
                            const GURL& relay_url,
                            UrlRequestCallback callback);

  const raw_ptr<network::mojom::NetworkContext> mojom_network_context_ =
      nullptr;
  const scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_ =
      nullptr;
  const bool use_staging_relay_ = false;

  URLLoaderList url_loaders_;

  base::WeakPtrFactory<NetworkClient> weak_ptr_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_BROWSER_SERVICE_NETWORK_CLIENT_H_
