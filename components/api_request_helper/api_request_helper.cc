/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/api_request_helper/api_request_helper.h"

#include <utility>

#include "net/base/load_flags.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace api_request_helper {

const unsigned int kRetriesCountOnNetworkChange = 1;

APIRequestHelper::APIRequestHelper(
    net::NetworkTrafficAnnotationTag annotation_tag,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : annotation_tag_(annotation_tag),
      url_loader_factory_(url_loader_factory) {}

APIRequestHelper::~APIRequestHelper() {}

void APIRequestHelper::Request(
    const std::string& method,
    const GURL& url,
    const std::string& payload,
    const std::string& payload_content_type,
    bool auto_retry_on_network_change,
    ResultCallback callback,
    const base::flat_map<std::string, std::string>& headers /* ={} */,
    size_t max_body_size /* =-1 */) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->load_flags = net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
                        net::LOAD_DO_NOT_SAVE_COOKIES;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  request->method = method;

  if (!headers.empty()) {
    for (auto entry : headers)
      request->headers.SetHeader(entry.first, entry.second);
  }

  auto url_loader =
      network::SimpleURLLoader::Create(std::move(request), annotation_tag_);
  if (!payload.empty()) {
    url_loader->AttachStringForUpload(payload, payload_content_type);
  }
  url_loader->SetRetryOptions(
      kRetriesCountOnNetworkChange,
      auto_retry_on_network_change
          ? network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE
          : network::SimpleURLLoader::RetryMode::RETRY_NEVER);
  url_loader->SetAllowHttpErrorResults(true);
  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(url_loader));
  if (max_body_size == -1u) {
    iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
        url_loader_factory_.get(),
        base::BindOnce(&APIRequestHelper::OnResponse, base::Unretained(this),
                       iter, std::move(callback)));
  } else {
    iter->get()->DownloadToString(
        url_loader_factory_.get(),
        base::BindOnce(&APIRequestHelper::OnResponse, base::Unretained(this),
                       iter, std::move(callback)),
        max_body_size);
  }
}

void APIRequestHelper::OnResponse(
    SimpleURLLoaderList::iterator iter,
    ResultCallback callback,
    const std::unique_ptr<std::string> response_body) {
  auto* loader = iter->get();
  auto response_code = -1;
  base::flat_map<std::string, std::string> headers;
  if (loader->ResponseInfo()) {
    auto headers_list = loader->ResponseInfo()->headers;
    if (headers_list) {
      response_code = headers_list->response_code();
      size_t iter = 0;
      std::string key;
      std::string value;
      while (headers_list->EnumerateHeaderLines(&iter, &key, &value)) {
        key = base::ToLowerASCII(key);
        headers[key] = value;
      }
    }
  }
  url_loaders_.erase(iter);
  std::move(callback).Run(response_code, response_body ? *response_body : "",
                          headers);
}

}  // namespace api_request_helper
