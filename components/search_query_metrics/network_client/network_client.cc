/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/search_query_metrics/network_client/network_client.h"

#include <cstddef>
#include <utility>

#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/memory/scoped_refptr.h"
#include "base/strings/string_util.h"
#include "brave/components/search_query_metrics/network_client/network_client_util.h"
#include "brave/components/search_query_metrics/network_client/oblivious_http_client_impl.h"
#include "brave/components/search_query_metrics/network_client/oblivious_http_feature.h"
#include "brave/components/search_query_metrics/network_client/oblivious_http_key_config.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom-data-view.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/oblivious_http_request.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace metrics {

namespace {

// Builds an `network::mojom::ObliviousHttpRequest` from the given parameters.
network::mojom::ObliviousHttpRequestPtr BuildObliviousHttpRequest(
    const GURL& relay_url,
    const std::string& key_config,
    const GURL& url,
    const std::string& content,
    const std::string& content_type,
    const std::string& method) {
  auto mojom_http_request = network::mojom::ObliviousHttpRequest::New();
  mojom_http_request->relay_url = relay_url;
  mojom_http_request->traffic_annotation =
      net::MutableNetworkTrafficAnnotationTag(GetNetworkTrafficAnnotationTag());
  mojom_http_request->timeout_duration = kOhttpTimeoutDuration.Get();
  mojom_http_request->key_config = key_config;
  mojom_http_request->resource_url = url;
  mojom_http_request->method = method;
  mojom_http_request->request_body =
      network::mojom::ObliviousHttpRequestBody::New(content, content_type);
  return mojom_http_request;
}

// Extracts all HTTP response headers from `net::HttpResponseHeaders` and
// returns them as a flat map with lowercased keys.
base::flat_map<std::string, std::string> ExtractHttpResponseHeaders(
    const scoped_refptr<net::HttpResponseHeaders>& http_response_headers) {
  CHECK(http_response_headers);

  size_t iter = 0;
  std::string key;
  std::string value;

  base::flat_map<std::string, std::string> headers;
  while (http_response_headers->EnumerateHeaderLines(&iter, &key, &value)) {
    key = base::ToLowerASCII(key);
    headers[key] = value;
  }

  return headers;
}

// Reports an error to the caller, including the URL and response code. The
// response code will be a `net::ERR_*` value if the request failed before
// receiving an HTTP response; otherwise, it will be a `net::HTTP_*` code.
void ReportError(const GURL& url,
                 int response_code,
                 SendRequestCallback callback) {
  // Forward the response to the original caller for handling.
  std::move(callback).Run(url, response_code, /*response_body=*/"",
                          /*headers=*/{});
}

}  // namespace

NetworkClient::NetworkClient(
    PrefService& local_state,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    network::NetworkContextGetter network_context_getter,
    bool use_ohttp_staging)
    : local_state_(local_state),
      url_loader_factory_(std::move(url_loader_factory)),
      network_context_getter_(std::move(network_context_getter)),
      oblivious_http_key_config_(std::make_unique<ObliviousHttpKeyConfig>(
          local_state_.get(),
          url_loader_factory_,
          ObliviousHttpKeyConfigUrl(use_ohttp_staging))),
      oblivious_http_relay_url_(ObliviousHttpRelayUrl(use_ohttp_staging)) {
  CHECK(oblivious_http_key_config_);

  // Fetch the OHTTP key config so the client is ready.
  if (kShouldSupportOhttp.Get()) {
    oblivious_http_key_config_->MaybeFetch();
  }
}

NetworkClient::~NetworkClient() = default;

void NetworkClient::SendRequest(const GURL& url,
                                const std::vector<std::string>& headers,
                                const std::string& content,
                                const std::string& content_type,
                                const std::string& method,
                                SendRequestCallback callback) {
  if (!kShouldSupportOhttp.Get()) {
    return HttpRequest(url, headers, content, content_type, method,
                       std::move(callback));
  }

  ObliviousHttpRequest(url, content, content_type, method,
                       oblivious_http_relay_url_, std::move(callback));
}

void NetworkClient::CancelRequests() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

///////////////////////////////////////////////////////////////////////////////

void NetworkClient::HttpRequest(const GURL& url,
                                const std::vector<std::string>& headers,
                                const std::string& content,
                                const std::string& content_type,
                                const std::string& method,
                                SendRequestCallback callback) {
  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = url;
  resource_request->method = method;
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;

  for (const auto& header : headers) {
    resource_request->headers.AddHeaderFromString(header);
  }

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(resource_request), GetNetworkTrafficAnnotationTag());
  auto* const raw_url_loader = url_loader.get();

  raw_url_loader->SetAllowHttpErrorResults(true);

  if (!content.empty()) {
    raw_url_loader->AttachStringForUpload(content, content_type);
  }

  raw_url_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&NetworkClient::HttpRequestCallback,
                     weak_ptr_factory_.GetWeakPtr(), std::move(url_loader),
                     std::move(callback)));
}

void NetworkClient::HttpRequestCallback(
    std::unique_ptr<network::SimpleURLLoader> url_loader,
    SendRequestCallback callback,
    std::optional<std::string> response_body) {
  CHECK(url_loader);

  const GURL& url = url_loader->GetFinalURL();

  const auto* const response = url_loader->ResponseInfo();
  if (!response || !response->headers) {
    // DNS failure, connection error, timeout etc.
    return ReportError(url, url_loader->NetError(), std::move(callback));
  }

  // Forward the response to the original caller for handling.
  std::move(callback).Run(url, response->headers->response_code(),
                          response_body.value_or(""),
                          ExtractHttpResponseHeaders(response->headers));
}

void NetworkClient::ObliviousHttpRequest(const GURL& url,
                                         const std::string& content,
                                         const std::string& content_type,
                                         const std::string& method,
                                         const GURL& relay_url,
                                         SendRequestCallback callback) {
  CHECK(url.is_valid());
  CHECK(!content_type.empty());
  CHECK(relay_url.is_valid());

  std::optional<std::string> key_config = oblivious_http_key_config_->Get();
  if (!key_config) {
    // The OHTTP key config is not ready. This can occur while a fetch is still
    // in progress after first run or after the key config is invalidated.
    VLOG(1) << "OHTTP key config is not ready";
    return ReportError(url, net::ERR_FAILED, std::move(callback));
  }

  network::mojom::NetworkContext* const mojom_network_context =
      network_context_getter_.Run();
  if (!mojom_network_context) {
    VLOG(0) << "Network context is unavailable";
    return ReportError(url, net::ERR_FAILED, std::move(callback));
  }

  mojo::PendingRemote<network::mojom::ObliviousHttpClient> mojom_pending_remote;
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<ObliviousHttpClientImpl>(
          url,
          base::BindOnce(&NetworkClient::ObliviousHttpRequestCallback,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback))),
      mojom_pending_remote.InitWithNewPipeAndPassReceiver());

  mojom_network_context->GetViaObliviousHttp(
      BuildObliviousHttpRequest(relay_url, *key_config, url, content,
                                content_type, method),
      std::move(mojom_pending_remote));
}

void NetworkClient::ObliviousHttpRequestCallback(
    SendRequestCallback callback,
    const GURL& url,
    int response_code,
    const std::string& response_body,
    const base::flat_map<std::string, std::string>& response_headers) {
  if (response_code == net::HTTP_UNPROCESSABLE_CONTENT) {
    // The OHTTP key config is invalid or has been rotated, so refetch it.
    oblivious_http_key_config_->Refetch();
  }

  // Forward the response to the original caller for handling.
  std::move(callback).Run(url, response_code, response_body, response_headers);
}

}  // namespace metrics
