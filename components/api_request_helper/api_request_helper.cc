/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/api_request_helper/api_request_helper.h"

#include <utility>
#include <vector>

#include "base/json/json_writer.h"
#include "base/strings/string_split.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
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
    : annotation_tag_(annotation_tag), url_loader_factory_(url_loader_factory) {
  data_decoder_ = std::make_unique<data_decoder::DataDecoder>();
  data_decoder_->GetService()->BindJsonParser(
      json_parser_.BindNewPipeAndPassReceiver());
}

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
  auto loader_wrapper_handler = CreateLoaderWrapperHandler(
      method, url, payload, payload_content_type,
      request_options.auto_retry_on_network_change,
      request_options.enable_cache, true /* allow_http_error_result*/, headers);

  if (request_options.timeout) {
    loader_wrapper_handler->url_loader_->SetTimeoutDuration(
        request_options.timeout.value());
  }

  auto iter = url_loaders_.insert(url_loaders_.begin(),
                                  std::move(loader_wrapper_handler));

  if (request_options.max_body_size == -1u) {
    iter->get()->url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
        url_loader_factory_.get(),
        base::BindOnce(&APIRequestHelper::OnResponse,
                       weak_ptr_factory_.GetWeakPtr(), iter,
                       std::move(callback), std::move(conversion_callback)));
  } else {
    iter->get()->url_loader_->DownloadToString(
        url_loader_factory_.get(),
        base::BindOnce(&APIRequestHelper::OnResponse,
                       weak_ptr_factory_.GetWeakPtr(), iter,
                       std::move(callback), std::move(conversion_callback)),
        request_options.max_body_size);
  }

  return iter;
}

APIRequestHelper::Ticket APIRequestHelper::RequestSSE(
    const std::string& method,
    const GURL& url,
    const std::string& payload,
    const std::string& payload_content_type,
    DataReceivedCallback data_received_callback,
    DataCompletedCallback data_completed_callback,
    const base::flat_map<std::string, std::string>& headers,
    const APIRequestOptions& request_options) {
  auto loader_wrapper_handler = CreateLoaderWrapperHandler(
      method, url, payload, payload_content_type,
      request_options.auto_retry_on_network_change,
      request_options.enable_cache, true /* allow_http_error_result*/, headers);

  auto iter = url_loaders_.insert(url_loaders_.begin(),
                                  std::move(loader_wrapper_handler));

  iter->get()->data_received_callback_ = std::move(data_received_callback);
  iter->get()->data_completed_callback_ = std::move(data_completed_callback);

  iter->get()->url_loader_->DownloadAsStream(url_loader_factory_.get(),
                                             iter->get());

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
      CreateLoaderWrapperHandler({}, url, payload, payload_content_type,
                                 request_options.auto_retry_on_network_change,
                                 request_options.enable_cache,
                                 false /*allow_http_error_result*/, headers));
  iter->get()->url_loader_->DownloadToFile(
      url_loader_factory_.get(),
      base::BindOnce(&APIRequestHelper::OnDownload,
                     weak_ptr_factory_.GetWeakPtr(), iter, std::move(callback)),
      path);

  return iter;
}

void APIRequestHelper::Cancel(const Ticket& ticket) {
  url_loaders_.erase(ticket);
}

void APIRequestHelper::Cancel(LoaderWrapperHandler* request_handler) {
  Ticket iter = base::ranges::find_if(
      url_loaders_,
      [request_handler](std::unique_ptr<LoaderWrapperHandler>& handler) {
        return handler.get() == request_handler;
      });
  if (iter != url_loaders_.end()) {
    Cancel(iter);
  }
}

std::unique_ptr<APIRequestHelper::LoaderWrapperHandler>
APIRequestHelper::CreateLoaderWrapperHandler(
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

  auto loader_wrapper_handler = std::make_unique<LoaderWrapperHandler>(this);
  loader_wrapper_handler->RegisterURLLoader(std::move(url_loader));

  return loader_wrapper_handler;
}

void APIRequestHelper::OnResponse(
    Ticket iter,
    ResultCallback callback,
    ResponseConversionCallback conversion_callback,
    const std::unique_ptr<std::string> response_body) {
  std::unique_ptr<network::SimpleURLLoader> loader =
      std::move(iter->get()->url_loader_);
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

  Cancel(iter);

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

void APIRequestHelper::OnDownload(Ticket iter,
                                  DownloadCallback callback,
                                  base::FilePath path) {
  std::unique_ptr<network::SimpleURLLoader> loader =
      std::move(iter->get()->url_loader_);
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

  Cancel(iter);
  std::move(callback).Run(path, std::move(headers));
}

APIRequestHelper::LoaderWrapperHandler::LoaderWrapperHandler(
    APIRequestHelper* api_request_helper)
    : api_request_helper_(api_request_helper) {}
APIRequestHelper::LoaderWrapperHandler::~LoaderWrapperHandler() = default;

void APIRequestHelper::LoaderWrapperHandler::OnDataReceived(
    base::StringPiece string_piece,
    base::OnceClosure resume) {
  // Binding a remote will guarantee the order of the messages being sent, so we
  // must ensure that it is connected.
  DCHECK(api_request_helper_->data_decoder_);
  DCHECK(api_request_helper_->json_parser_.is_connected());

  // We split the string into multiple chunks because there are cases where
  // multiple chunks are received in a single call.
  std::vector<std::string> stream_data = base::SplitString(
      string_piece, "\r\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  static constexpr char kDataPrefix[] = "data: {";

  for (const auto& data : stream_data) {
    if (!base::StartsWith(data, kDataPrefix)) {
      continue;
    }

    std::string json = data.substr(strlen(kDataPrefix) - 1);

    api_request_helper_->data_decoder_->ParseJson(
        json,
        base::BindOnce(
            &APIRequestHelper::LoaderWrapperHandler::OnParseJsonFromStream,
            weak_ptr_factory_.GetWeakPtr()));
  }

  std::move(resume).Run();
}

void APIRequestHelper::LoaderWrapperHandler::OnComplete(bool success) {
  DCHECK(data_completed_callback_);
  DVLOG(1) << "[[" << __func__ << "]]"
           << " Response completed\n";

  int response_code = -1;

  if (url_loader_ && url_loader_->ResponseInfo()->headers) {
    response_code = url_loader_->ResponseInfo()->headers->response_code();
  }

  std::move(data_completed_callback_).Run(success, response_code);
  api_request_helper_->Cancel(this);
}

void APIRequestHelper::LoaderWrapperHandler::OnRetry(
    base::OnceClosure start_retry) {
  // Retries are not enabled for these requests.
  NOTREACHED();
}

void APIRequestHelper::LoaderWrapperHandler::OnParseJsonFromStream(
    data_decoder::DataDecoder::ValueOrError result) {
  DCHECK(data_received_callback_);

  if (!result.has_value()) {
    return;
  }

  if (const std::string* completion = result->FindStringKey("completion")) {
    data_received_callback_.Run(*completion);
  }
}

void APIRequestHelper::LoaderWrapperHandler::RegisterURLLoader(
    std::unique_ptr<network::SimpleURLLoader> loader) {
  url_loader_ = std::move(loader);
}

}  // namespace api_request_helper
