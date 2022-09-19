/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/api_request_helper/api_request_helper.h"

#include <utility>

#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "services/data_decoder/public/cpp/json_sanitizer.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace api_request_helper {

namespace {

void OnSanitize(const int http_code,
                const base::flat_map<std::string, std::string>& headers,
                int error_code,
                GURL final_url,
                APIRequestHelper::ResultCallback result_callback,
                data_decoder::JsonSanitizer::Result result) {
  std::string response_body;
  if (result.error) {
    VLOG(1) << "Response validation error:" << *result.error;
    std::move(result_callback)
        .Run(APIRequestResult(http_code, "", std::move(headers), error_code,
                              final_url));
    return;
  }

  if (result.value.has_value()) {
    response_body = result.value.value();
  }

  std::move(result_callback)
      .Run(APIRequestResult(http_code, std::move(response_body),
                            std::move(headers), error_code, final_url));
}

const unsigned int kRetriesCountOnNetworkChange = 1;

}  // namespace

APIRequestResult::APIRequestResult() = default;
APIRequestResult::APIRequestResult(
    int response_code,
    std::string body,
    base::flat_map<std::string, std::string> headers,
    int error_code,
    GURL final_url)
    : final_url_(final_url),
      error_code_(error_code),
      response_code_(response_code),
      body_(body),
      headers_(headers) {}
APIRequestResult::APIRequestResult(const APIRequestResult&) = default;
APIRequestResult& APIRequestResult::operator=(const APIRequestResult&) =
    default;
APIRequestResult::APIRequestResult(APIRequestResult&&) = default;
APIRequestResult& APIRequestResult::operator=(APIRequestResult&&) = default;
APIRequestResult::~APIRequestResult() = default;

bool APIRequestResult::Is2XXResponseCode() const {
  return response_code_ >= 200 && response_code_ <= 299;
}

APIRequestHelper::APIRequestHelper(
    net::NetworkTrafficAnnotationTag annotation_tag,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : annotation_tag_(annotation_tag),
      url_loader_factory_(url_loader_factory) {}

APIRequestHelper::~APIRequestHelper() = default;

APIRequestHelper::Ticket APIRequestHelper::Request(
    const std::string& method,
    const GURL& url,
    const std::string& payload,
    const std::string& payload_content_type,
    bool auto_retry_on_network_change,
    ResultCallback callback,
    const base::flat_map<std::string, std::string>& headers,
    size_t max_body_size /* = -1u */,
    ResponseConversionCallback conversion_callback) {
  auto iter = url_loaders_.insert(
      url_loaders_.begin(),
      CreateLoader(method, url, payload, payload_content_type,
                   auto_retry_on_network_change,
                   true /* allow_http_error_result*/, headers));
  if (max_body_size == -1u) {
    iter->get()->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
        url_loader_factory_.get(),
        base::BindOnce(&APIRequestHelper::OnResponse,
                       weak_ptr_factory_.GetWeakPtr(), iter,
                       std::move(callback), std::move(conversion_callback)));
  } else {
    iter->get()->DownloadToString(
        url_loader_factory_.get(),
        base::BindOnce(&APIRequestHelper::OnResponse,
                       weak_ptr_factory_.GetWeakPtr(), iter,
                       std::move(callback), std::move(conversion_callback)),
        max_body_size);
  }

  return iter;
}

APIRequestHelper::Ticket APIRequestHelper::Download(
    const GURL& url,
    const std::string& payload,
    const std::string& payload_content_type,
    bool auto_retry_on_network_change,
    const base::FilePath& path,
    DownloadCallback callback,
    const base::flat_map<std::string, std::string>& headers) {
  auto iter = url_loaders_.insert(
      url_loaders_.begin(),
      CreateLoader({}, url, payload, payload_content_type,
                   auto_retry_on_network_change,
                   false /*allow_http_error_result*/, headers));
  iter->get()->DownloadToFile(
      url_loader_factory_.get(),
      base::BindOnce(&APIRequestHelper::OnDownload,
                     weak_ptr_factory_.GetWeakPtr(), iter, std::move(callback)),
      path);

  return iter;
}

void APIRequestHelper::Cancel(const Ticket& ticket) {
  url_loaders_.erase(ticket);
}

std::unique_ptr<network::SimpleURLLoader> APIRequestHelper::CreateLoader(
    const std::string& method,
    const GURL& url,
    const std::string& payload,
    const std::string& payload_content_type,
    bool auto_retry_on_network_change,
    bool allow_http_error_result,
    const base::flat_map<std::string, std::string>& headers) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->load_flags = net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE |
                        net::LOAD_DO_NOT_SAVE_COOKIES;
  request->credentials_mode = network::mojom::CredentialsMode::kOmit;
  if (!method.empty())
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
  url_loader->SetAllowHttpErrorResults(allow_http_error_result);
  return url_loader;
}

void APIRequestHelper::OnResponse(
    SimpleURLLoaderList::iterator iter,
    ResultCallback callback,
    ResponseConversionCallback conversion_callback,
    const std::unique_ptr<std::string> response_body) {
  auto* loader = iter->get();
  auto response_code = -1;
  auto error_code = loader->NetError();
  auto final_url = loader->GetFinalURL();
  base::flat_map<std::string, std::string> headers;
  if (loader->ResponseInfo()) {
    auto headers_list = loader->ResponseInfo()->headers;
    if (headers_list) {
      response_code = headers_list->response_code();
      size_t header_iter = 0;
      std::string key;
      std::string value;
      while (headers_list->EnumerateHeaderLines(&header_iter, &key, &value)) {
        key = base::ToLowerASCII(key);
        headers[key] = value;
      }
    }
  }

  url_loaders_.erase(iter);
  if (!response_body) {
    std::move(callback).Run(APIRequestResult(
        response_code, "", std::move(headers), error_code, final_url));
    return;
  }
  auto& raw_body = *response_body;
  if (conversion_callback) {
    auto converted_body = std::move(conversion_callback).Run(raw_body);
    if (!converted_body) {
      std::move(callback).Run(APIRequestResult(
          422, std::move(raw_body), std::move(headers), error_code, final_url));
      return;
    }
    raw_body = converted_body.value();
  }

  data_decoder::JsonSanitizer::Sanitize(
      std::move(raw_body),
      base::BindOnce(&OnSanitize, response_code, std::move(headers), error_code,
                     final_url, std::move(callback)));
}

void APIRequestHelper::OnDownload(SimpleURLLoaderList::iterator iter,
                                  DownloadCallback callback,
                                  base::FilePath path) {
  url_loaders_.erase(iter);
  std::move(callback).Run(path);
}

}  // namespace api_request_helper
