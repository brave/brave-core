// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/components/brave_private_cdn/private_cdn_request_helper.h"

#include <utility>

#include "brave/components/brave_private_cdn/headers.h"
#include "net/base/load_flags.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace brave_private_cdn {

namespace {

const unsigned int kRetriesCountOnNetworkChange = 3;

}  // namespace

PrivateCDNRequestHelper::PrivateCDNRequestHelper(
    net::NetworkTrafficAnnotationTag annotation_tag,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : annotation_tag_(annotation_tag),
      url_loader_factory_(std::move(url_loader_factory)) {}

PrivateCDNRequestHelper::~PrivateCDNRequestHelper() = default;

void PrivateCDNRequestHelper::DownloadToString(
    const GURL& url,
    DownloadToStringCallback callback,
    bool auto_retry_on_network_change /* = true */,
    size_t max_body_size /* = 5 * 1024 * 1024 */) {
  // Do not allow unbounded max body
  DCHECK_NE(max_body_size, -1u);
  // Create private request
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  // Load flags allow cache, although we may want to accept a param for this,
  // if we are requesting static urls which do not support etag.
  // Don't send or save cookies
  request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->method = net::HttpRequestHeaders::kGetMethod;
  // Don't send any identifying information, such as language or user agent
  for (const auto& entry : brave::private_cdn_headers)
    request->headers.SetHeader(entry.first, entry.second);

  auto url_loader =
      network::SimpleURLLoader::Create(std::move(request), annotation_tag_);
  // Private CDN requests should be simple data reads, so allow retries without
  // fear of repeated operations like we would have with an API.
  url_loader->SetRetryOptions(
      kRetriesCountOnNetworkChange,
      auto_retry_on_network_change
          ? network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE
          : network::SimpleURLLoader::RetryMode::RETRY_NEVER);
  url_loader->SetAllowHttpErrorResults(true);
  // Send request
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));
  iter->get()->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&PrivateCDNRequestHelper::OnResponse,
                     weak_ptr_factory_.GetWeakPtr(), iter, std::move(callback)),
      max_body_size);
}

void PrivateCDNRequestHelper::OnResponse(
    SimpleURLLoaderList::iterator iter,
    DownloadToStringCallback callback,
    const std::unique_ptr<std::string> response_body) {
  // For now all the caller needs is response code and body as string
  auto* loader = iter->get();
  auto response_code = -1;
  if (loader->ResponseInfo()) {
    auto headers_list = loader->ResponseInfo()->headers;
    if (headers_list) {
      response_code = headers_list->response_code();
    }
  }
  url_loaders_.erase(iter);
  std::move(callback).Run(response_code,
                          (response_body != nullptr) ? *response_body : "");
}

}  // namespace brave_private_cdn
