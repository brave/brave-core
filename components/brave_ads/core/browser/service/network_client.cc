/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/network_client.h"

#include <utility>

#include "base/check.h"
#include "base/containers/flat_map.h"
#include "base/functional/bind.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/browser/service/network_client_util.h"
#include "brave/components/brave_ads/core/browser/service/oblivious_http_client_impl.h"
#include "brave/components/brave_ads/core/browser/service/oblivious_http_feature.h"
#include "brave/components/brave_ads/core/browser/service/oblivious_http_key_config.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/self_owned_receiver.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom-data-view.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/oblivious_http_request.mojom.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace brave_ads {

namespace {

void ReportError(const GURL& url,
                 int response_code,
                 UrlRequestCallback callback) {
  auto mojom_url_response = mojom::UrlResponseInfo::New();
  mojom_url_response->url = url;
  mojom_url_response->code = response_code;
  std::move(callback).Run(std::move(mojom_url_response));
}

}  // namespace

NetworkClient::NetworkClient(
    PrefService* local_state,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    network::NetworkContextGetter network_context_getter,
    bool use_ohttp_staging)
    : local_state_(local_state),
      url_loader_factory_(std::move(url_loader_factory)),
      network_context_getter_(std::move(network_context_getter)),
      oblivious_http_key_config_(std::make_unique<ObliviousHttpKeyConfig>(
          local_state_,
          url_loader_factory_,
          ObliviousHttpKeyConfigUrl(use_ohttp_staging))),
      oblivious_http_relay_url_(ObliviousHttpRelayUrl(use_ohttp_staging)) {
  CHECK(local_state_);
  CHECK(oblivious_http_key_config_);

  // Fetch the OHTTP key config so the client is ready.
  oblivious_http_key_config_->MaybeFetch();
}

NetworkClient::~NetworkClient() = default;

void NetworkClient::UrlRequest(mojom::UrlRequestInfoPtr mojom_url_request,
                               UrlRequestCallback callback) {
  CHECK(mojom_url_request);

  const bool use_ohttp =
      kShouldSupportOhttp.Get() && mojom_url_request->use_ohttp;

  if (!use_ohttp) {
    return HttpRequest(std::move(mojom_url_request), std::move(callback));
  }

  ObliviousHttpRequest(std::move(mojom_url_request), oblivious_http_relay_url_,
                       std::move(callback));
}

void NetworkClient::CancelRequests() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

///////////////////////////////////////////////////////////////////////////////

void NetworkClient::HttpRequest(mojom::UrlRequestInfoPtr mojom_url_request,
                                UrlRequestCallback callback) {
  CHECK(mojom_url_request);

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = mojom_url_request->url;
  resource_request->method = ToString(mojom_url_request->method);
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;

  for (const auto& header : mojom_url_request->headers) {
    resource_request->headers.AddHeaderFromString(header);
  }

  auto url_loader = network::SimpleURLLoader::Create(
      std::move(resource_request), GetNetworkTrafficAnnotationTag());
  auto* raw_url_loader = url_loader.get();

  raw_url_loader->SetAllowHttpErrorResults(true);

  if (!mojom_url_request->content.empty()) {
    raw_url_loader->AttachStringForUpload(mojom_url_request->content,
                                          mojom_url_request->content_type);
  }

  raw_url_loader->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
      url_loader_factory_.get(),
      base::BindOnce(&NetworkClient::HttpRequestCallback,
                     weak_ptr_factory_.GetWeakPtr(), std::move(url_loader),
                     std::move(callback)));
}

void NetworkClient::HttpRequestCallback(
    std::unique_ptr<network::SimpleURLLoader> url_loader,
    UrlRequestCallback callback,
    std::optional<std::string> response_body) {
  CHECK(url_loader);

  const GURL& url = url_loader->GetFinalURL();

  const auto* response = url_loader->ResponseInfo();
  if (!response || !response->headers) {
    // DNS failure, connection error, timeout etc.
    return ReportError(url, url_loader->NetError(), std::move(callback));
  }

  auto mojom_url_response = mojom::UrlResponseInfo::New();
  mojom_url_response->url = url;
  mojom_url_response->code = response->headers->response_code();
  mojom_url_response->body = response_body.value_or("");
  mojom_url_response->headers = ExtractHttpResponseHeaders(response->headers);
  std::move(callback).Run(std::move(mojom_url_response));
}

void NetworkClient::ObliviousHttpRequest(
    mojom::UrlRequestInfoPtr mojom_url_request,
    const GURL& relay_url,
    UrlRequestCallback callback) {
  CHECK(mojom_url_request);
  CHECK(!mojom_url_request->content_type.empty());
  CHECK(relay_url.is_valid());

  std::optional<std::string> key_config = oblivious_http_key_config_->Get();
  if (!key_config) {
    // The OHTTP key config is not ready. This can occur while a fetch is still
    // in progress after first run or after the key config is invalidated.
    VLOG(6) << "OHTTP key config is not ready";
    return ReportError(mojom_url_request->url, net::ERR_FAILED,
                       std::move(callback));
  }

  mojo::PendingRemote<network::mojom::ObliviousHttpClient> mojom_pending_remote;
  mojo::MakeSelfOwnedReceiver(
      std::make_unique<ObliviousHttpClientImpl>(
          mojom_url_request->url,
          base::BindOnce(&NetworkClient::ObliviousHttpRequestCallback,
                         weak_ptr_factory_.GetWeakPtr(), std::move(callback))),
      mojom_pending_remote.InitWithNewPipeAndPassReceiver());

  network_context_getter_.Run()->GetViaObliviousHttp(
      BuildObliviousHttpRequest(relay_url, *key_config, mojom_url_request),
      std::move(mojom_pending_remote));
}

void NetworkClient::ObliviousHttpRequestCallback(
    UrlRequestCallback callback,
    mojom::UrlResponseInfoPtr mojom_url_response) {
  if (mojom_url_response &&
      mojom_url_response->code == net::HTTP_UNPROCESSABLE_CONTENT) {
    // The OHTTP key config is invalid or has been rotated, so refetch it.
    oblivious_http_key_config_->InvalidateAndFetch();
  }

  // Forward the response to the original caller for handling.
  std::move(callback).Run(std::move(mojom_url_response));
}

}  // namespace brave_ads
