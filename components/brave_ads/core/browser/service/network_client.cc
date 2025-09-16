/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/network_client.h"

#include <utility>

#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/browser/service/ads_service_feature.h"
#include "brave/components/brave_ads/core/browser/service/network_client_constants.h"
#include "brave/components/brave_ads/core/browser/service/network_client_oblivious_http_impl.h"
#include "brave/components/brave_ads/core/browser/service/network_client_oblivious_http_key_config.h"
#include "brave/components/brave_ads/core/browser/service/network_client_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/base/net_errors.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/oblivious_http_request.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

void ReportError(const GURL& url, int error_code, UrlRequestCallback callback) {
  auto mojom_url_response = mojom::UrlResponseInfo::New();
  mojom_url_response->url = url;
  mojom_url_response->status_code = error_code;
  std::move(callback).Run(std::move(mojom_url_response));
}

}  // namespace

NetworkClient::NetworkClient(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    network::mojom::NetworkContext* mojom_network_context,
    bool use_staging)
    : url_loader_factory_(std::move(url_loader_factory)),
      mojom_network_context_(mojom_network_context),
      use_staging_(use_staging) {
  FetchObliviousHttpKeyConfig();
}

NetworkClient::~NetworkClient() = default;

void NetworkClient::UrlRequest(mojom::UrlRequestInfoPtr mojom_url_request,
                               UrlRequestCallback callback) {
  // Decide whether to use OHTTP based on the feature flag and the request's
  // `use_ohttp` parameter.
  const bool use_ohttp =
      kShouldSupportOhttp.Get() && mojom_url_request->use_ohttp;

  if (!use_ohttp) {
    return HttpRequest(std::move(mojom_url_request), std::move(callback));
  }

  ObliviousHttpRequest(
      std::move(mojom_url_request),
      GURL(use_staging_ ? kStagingRelayUrl : kProductionRelayUrl),
      std::move(callback));
}

void NetworkClient::CancelRequests() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  url_loaders_.clear();
}

///////////////////////////////////////////////////////////////////////////////

void NetworkClient::HttpRequest(mojom::UrlRequestInfoPtr mojom_url_request,
                                UrlRequestCallback callback) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = mojom_url_request->url;
  resource_request->method = ToString(mojom_url_request->method);
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;

  for (const auto& header : mojom_url_request->headers) {
    resource_request->headers.AddHeaderFromString(header);
  }

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(resource_request), GetNetworkTrafficAnnotationTag());

  url_loader->SetAllowHttpErrorResults(true);

  if (!mojom_url_request->content.empty()) {
    url_loader->AttachStringForUpload(mojom_url_request->content,
                                      mojom_url_request->content_type);
  }

  auto* url_loader_copy = url_loader.get();
  url_loaders_.insert(std::move(url_loader));

  url_loader_copy->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&NetworkClient::HttpRequestCallback,
                     weak_ptr_factory_.GetWeakPtr(), url_loader_copy,
                     std::move(callback)));
}

void NetworkClient::HttpRequestCallback(
    network::SimpleURLLoader* url_loader,
    UrlRequestCallback callback,
    std::optional<std::string> url_response_body) {
  CHECK(url_loader);

  auto iter = url_loaders_.find(url_loader);
  CHECK(iter != url_loaders_.cend());
  auto owned_url_loader = std::move(*iter);
  url_loaders_.erase(iter);

  const GURL& url = owned_url_loader->GetFinalURL();

  const auto* response = owned_url_loader->ResponseInfo();
  if (!response || !response->headers) {
    return ReportError(url, net::ERR_FAILED, std::move(callback));
  }

  const auto response_headers = response->headers;

  auto mojom_url_response = mojom::UrlResponseInfo::New();
  mojom_url_response->url = url;
  mojom_url_response->status_code = response_headers->response_code();
  mojom_url_response->body = url_response_body.value_or("");
  mojom_url_response->headers = ExtractHttpResponseHeaders(response_headers);
  std::move(callback).Run(std::move(mojom_url_response));
}

void NetworkClient::FetchObliviousHttpKeyConfig() {
  oblivious_http_key_config_ =
      std::make_unique<NetworkClientObliviousHttpKeyConfig>(
          url_loader_factory_,
          GURL(use_staging_ ? kStagingKeyConfigUrl : kProductionKeyConfigUrl));
  oblivious_http_key_config_->Fetch();
}

void NetworkClient::ObliviousHttpRequest(
    mojom::UrlRequestInfoPtr mojom_url_request,
    const GURL& relay_url,
    UrlRequestCallback callback) {
  CHECK(relay_url.is_valid());

  if (!mojom_network_context_) {
    return ReportError(mojom_url_request->url, net::ERR_UNEXPECTED,
                       std::move(callback));
  }

  std::optional<std::string> key_config =
      oblivious_http_key_config_->KeyConfig();
  if (!key_config) {
    return ReportError(mojom_url_request->url, net::ERR_FAILED,
                       std::move(callback));
  }

  auto mojom_http_request = network::mojom::ObliviousHttpRequest::New();
  mojom_http_request->key_config = *key_config;
  mojom_http_request->relay_url = relay_url;
  mojom_http_request->traffic_annotation =
      net::MutableNetworkTrafficAnnotationTag(GetNetworkTrafficAnnotationTag());
  mojom_http_request->timeout_duration = kOhttpTimeoutDuration.Get();
  mojom_http_request->resource_url = mojom_url_request->url;
  mojom_http_request->method = ToString(mojom_url_request->method);

  // TODO(tmancey): Should we not fail if content type is not set, rather than
  // defaulting to `application/json`?
  const std::string content_type = !mojom_url_request->content_type.empty()
                                       ? mojom_url_request->content_type
                                       : "application/json";
  mojom_http_request->request_body =
      network::mojom::ObliviousHttpRequestBody::New(mojom_url_request->content,
                                                    content_type);

  mojo::PendingRemote<network::mojom::ObliviousHttpClient> mojom_pending_remote;
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<ObliviousHttpClientImpl>(mojom_url_request->url,
                                                std::move(callback)),
      mojom_pending_remote.InitWithNewPipeAndPassReceiver());

  mojom_network_context_->GetViaObliviousHttp(std::move(mojom_http_request),
                                              std::move(mojom_pending_remote));
}

}  // namespace brave_ads
