/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/browser/service/network_client.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/strings/string_util.h"
#include "brave/components/brave_ads/core/browser/service/network_client_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/fetch_api.mojom-data-view.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

namespace brave_ads {

namespace {

void ReportError(const GURL& url,
                 int response_code,
                 SendRequestCallback callback) {
  auto mojom_url_response = mojom::UrlResponseInfo::New();
  mojom_url_response->url = url;
  mojom_url_response->code = response_code;
  std::move(callback).Run(std::move(mojom_url_response));
}

}  // namespace

NetworkClient::NetworkClient(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    network::NetworkContextGetter network_context_getter)
    : url_loader_factory_(std::move(url_loader_factory)),
      network_context_getter_(std::move(network_context_getter)) {}

NetworkClient::~NetworkClient() = default;

void NetworkClient::SendRequest(mojom::UrlRequestInfoPtr mojom_url_request,
                                SendRequestCallback callback) {
  CHECK(mojom_url_request);

  return HttpRequest(std::move(mojom_url_request), std::move(callback));
}

void NetworkClient::CancelRequests() {
  weak_ptr_factory_.InvalidateWeakPtrs();
}

///////////////////////////////////////////////////////////////////////////////

void NetworkClient::HttpRequest(mojom::UrlRequestInfoPtr mojom_url_request,
                                SendRequestCallback callback) {
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
    SendRequestCallback callback,
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

  // Forward the response to the original caller for handling.
  std::move(callback).Run(std::move(mojom_url_response));
}

}  // namespace brave_ads
