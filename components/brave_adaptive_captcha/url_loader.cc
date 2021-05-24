/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_adaptive_captcha/url_loader.h"

#include <utility>

#include "chrome/browser/browser_process.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/storage_partition.h"
#include "services/network/public/cpp/resource_request.h"

// Maximum size of the response in bytes.
const int kMaxResponseSizeBytes = 1024 * 1024;

namespace {

std::string UrlMethodToRequestType(
    brave_adaptive_captcha::UrlLoader::UrlMethod method) {
  switch (method) {
    case brave_adaptive_captcha::UrlLoader::UrlMethod::GET: {
      return "GET";
    }

    case brave_adaptive_captcha::UrlLoader::UrlMethod::POST: {
      return "POST";
    }

    case brave_adaptive_captcha::UrlLoader::UrlMethod::PUT: {
      return "PUT";
    }
  }
}

}  // namespace

namespace brave_adaptive_captcha {

UrlLoader::UrlRequest::UrlRequest() {}

UrlLoader::UrlRequest::~UrlRequest() = default;

UrlLoader::UrlResponse::UrlResponse() {}

UrlLoader::UrlResponse::~UrlResponse() = default;

UrlLoader::UrlLoader(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : url_loader_factory_(std::move(url_loader_factory)) {}

UrlLoader::~UrlLoader() = default;

void UrlLoader::Load(const UrlRequest& url_request,
                     OnUrlRequestComplete callback) {
  net::NetworkTrafficAnnotationTag traffic_annotation =
      net::DefineNetworkTrafficAnnotation("brave_adaptive_captcha_service", R"(
        semantics {
          sender:
            "Brave Adaptive Captcha service"
          description:
            "Fetches CAPTCHA data from Brave."
          trigger:
            "The Brave service indicates that it's time to solve a CAPTCHA."
          data: "Brave CAPTCHA data."
          destination: WEBSITE
        }
        policy {
          cookies_allowed: NO
          setting:
            "This feature cannot be disabled by settings."
          policy_exception_justification:
            "Not implemented."
        })");

  auto resource_request = std::make_unique<network::ResourceRequest>();
  resource_request->url = GURL(url_request.url);
  resource_request->method = UrlMethodToRequestType(url_request.method);
  resource_request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  resource_request->load_flags = url_request.load_flags;
  for (const auto& header : url_request.headers) {
    resource_request->headers.AddHeaderFromString(header);
  }

  auto simple_url_loader = network::SimpleURLLoader::Create(
      std::move(resource_request), traffic_annotation);
  simple_url_loader->SetRetryOptions(
      1, network::SimpleURLLoader::RetryMode::RETRY_ON_NETWORK_CHANGE);
  simple_url_loader->SetAllowHttpErrorResults(true);
  if (!url_request.content.empty()) {
    simple_url_loader->AttachStringForUpload(url_request.content,
                                             url_request.content_type);
  }
  auto simple_url_loader_it = simple_url_loaders_.insert(
      simple_url_loaders_.end(), std::move(simple_url_loader));
  simple_url_loader_it->get()->DownloadToString(
      url_loader_factory_.get(),
      base::BindOnce(&UrlLoader::OnSimpleUrlLoaderComplete,
                     base::Unretained(this), simple_url_loader_it,
                     std::move(callback)),
      kMaxResponseSizeBytes);
}

void UrlLoader::OnSimpleUrlLoaderComplete(
    SimpleURLLoaderList::iterator iter,
    OnUrlRequestComplete callback,
    std::unique_ptr<std::string> response_body) {
  auto simple_url_loader = std::move(*iter);
  simple_url_loaders_.erase(iter);

  UrlResponse url_response;

  url_response.body = response_body ? *response_body : "";

  if (simple_url_loader->NetError() != net::OK) {
    url_response.error = net::ErrorToString(simple_url_loader->NetError());
  }

  int response_code = -1;
  if (simple_url_loader->ResponseInfo() &&
      simple_url_loader->ResponseInfo()->headers) {
    response_code = simple_url_loader->ResponseInfo()->headers->response_code();
  }
  url_response.status_code = response_code;

  url_response.url = simple_url_loader->GetFinalURL().spec();

  if (simple_url_loader->ResponseInfo()) {
    scoped_refptr<net::HttpResponseHeaders> headers =
        simple_url_loader->ResponseInfo()->headers;

    if (headers) {
      size_t iter = 0;
      std::string key;
      std::string value;
      while (headers->EnumerateHeaderLines(&iter, &key, &value)) {
        key = base::ToLowerASCII(key);
        url_response.headers[key] = value;
      }
    }
  }

  std::move(callback).Run(url_response);
}

}  // namespace brave_adaptive_captcha
