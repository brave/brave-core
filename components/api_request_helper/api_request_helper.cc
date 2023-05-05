/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/api_request_helper/api_request_helper.h"

#include <utility>

#include "base/json/json_writer.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace api_request_helper {

namespace {

void OnParseJsonIsolated(
    int http_code,
    const base::flat_map<std::string, std::string>& headers,
    int error_code,
    GURL final_url,
    APIRequestHelper::ResultCallback result_callback,
    data_decoder::DataDecoder::ValueOrError result) {
  if (!result.has_value()) {
    VLOG(1) << "Response validation error:" << result.error();
    std::move(result_callback)
        .Run(APIRequestResult(http_code, "", base::Value(), std::move(headers),
                              error_code, final_url));
    return;
  }

  if (!result.value().is_dict() && !result.value().is_list()) {
    VLOG(1) << "Response validation error: Invalid top-level type";
    std::move(result_callback)
        .Run(APIRequestResult(http_code, "", base::Value(), std::move(headers),
                              error_code, final_url));
    return;
  }

  std::string safe_json;
  if (!base::JSONWriter::Write(result.value(), &safe_json)) {
    VLOG(1) << "Response validation error: Encoding error";
    std::move(result_callback)
        .Run(APIRequestResult(http_code, "", base::Value(), std::move(headers),
                              error_code, final_url));
    return;
  }

  std::move(result_callback)
      .Run(APIRequestResult(http_code, safe_json, std::move(result.value()),
                            std::move(headers), error_code, final_url));
}

const unsigned int kRetriesCountOnNetworkChange = 1;

}  // namespace

APIRequestResult::APIRequestResult() = default;
APIRequestResult::APIRequestResult(
    int response_code,
    std::string body,
    base::Value value_body,
    base::flat_map<std::string, std::string> headers,
    int error_code,
    GURL final_url)
    : response_code_(response_code),
      body_(std::move(body)),
      value_body_(std::move(value_body)),
      headers_(std::move(headers)),
      error_code_(error_code),
      final_url_(std::move(final_url)) {}
APIRequestResult::APIRequestResult(APIRequestResult&&) = default;
APIRequestResult& APIRequestResult::operator=(APIRequestResult&&) = default;
APIRequestResult::~APIRequestResult() = default;

bool APIRequestResult::operator==(const APIRequestResult& other) const {
  auto tied = [](auto& v) {
    return std::tie(v.response_code_, v.body_, v.value_body_, v.headers_,
                    v.error_code_, v.final_url_);
  };
  return tied(*this) == tied(other);
}

bool APIRequestResult::operator!=(const APIRequestResult& other) const {
  return !(*this == other);
}

bool APIRequestResult::Is2XXResponseCode() const {
  return response_code_ >= 200 && response_code_ <= 299;
}

bool APIRequestResult::IsResponseCodeValid() const {
  return response_code_ >= 100 && response_code_ <= 599;
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
    ResultCallback callback,
    const base::flat_map<std::string, std::string>& headers,
    const APIRequestOptions& request_options,
    ResponseConversionCallback conversion_callback) {
  auto loader = CreateLoader(method, url, payload, payload_content_type,
                             request_options.auto_retry_on_network_change,
                             request_options.enable_cache,
                             true /* allow_http_error_result*/, headers);

  if (request_options.timeout) {
    loader->SetTimeoutDuration(request_options.timeout.value());
  }

  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(loader));
  if (request_options.max_body_size == -1u) {
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
        request_options.max_body_size);
  }

  return iter;
}

APIRequestHelper::Ticket APIRequestHelper::Request(
    const std::string& method,
    const GURL& url,
    const std::string& payload,
    const std::string& payload_content_type,
    network::SimpleURLLoaderStreamConsumer* consumer,
    const base::flat_map<std::string, std::string>& headers,
    const APIRequestOptions& request_options,
    network::SimpleURLLoader::OnResponseStartedCallback on_response_started_cb,
    network::SimpleURLLoader::DownloadProgressCallback download_progress_cb) {
  auto loader = CreateLoader(method, url, payload, payload_content_type,
                             request_options.auto_retry_on_network_change,
                             request_options.enable_cache,
                             true /* allow_http_error_result*/, headers);

  loader->SetOnResponseStartedCallback(std::move(on_response_started_cb));
  loader->SetOnDownloadProgressCallback(download_progress_cb);

  auto iter = url_loaders_.insert(url_loaders_.begin(), std::move(loader));
  iter->get()->DownloadAsStream(url_loader_factory_.get(), consumer);

  return iter;
}

APIRequestHelper::Ticket APIRequestHelper::Download(
    const GURL& url,
    const std::string& payload,
    const std::string& payload_content_type,
    const base::FilePath& path,
    DownloadCallback callback,
    const base::flat_map<std::string, std::string>& headers,
    const APIRequestOptions& request_options) {
  auto iter = url_loaders_.insert(
      url_loaders_.begin(),
      CreateLoader({}, url, payload, payload_content_type,
                   request_options.auto_retry_on_network_change,
                   request_options.enable_cache,
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
    bool enable_cache,
    bool allow_http_error_result,
    const base::flat_map<std::string, std::string>& headers) {
  auto request = std::make_unique<network::ResourceRequest>();
  request->url = url;
  request->load_flags = net::LOAD_DO_NOT_SAVE_COOKIES;
  if (!enable_cache) {
    request->load_flags =
        request->load_flags | net::LOAD_BYPASS_CACHE | net::LOAD_DISABLE_CACHE;
  }

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
    std::move(callback).Run(APIRequestResult(response_code, "", base::Value(),
                                             std::move(headers), error_code,
                                             final_url));
    return;
  }
  auto& raw_body = *response_body;
  if (conversion_callback) {
    auto converted_body = std::move(conversion_callback).Run(raw_body);
    if (!converted_body) {
      std::move(callback).Run(APIRequestResult(
          422, "", base::Value(), std::move(headers), error_code, final_url));
      return;
    }
    raw_body = converted_body.value();
  }

  data_decoder::DataDecoder::ParseJsonIsolated(
      raw_body,
      base::BindOnce(&OnParseJsonIsolated, response_code, std::move(headers),
                     error_code, final_url, std::move(callback)));
}

void APIRequestHelper::OnDownload(SimpleURLLoaderList::iterator iter,
                                  DownloadCallback callback,
                                  base::FilePath path) {
  auto* loader = iter->get();
  base::flat_map<std::string, std::string> headers;
  if (loader->ResponseInfo()) {
    auto headers_list = loader->ResponseInfo()->headers;
    if (headers_list) {
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
  std::move(callback).Run(path, std::move(headers));
}

}  // namespace api_request_helper
