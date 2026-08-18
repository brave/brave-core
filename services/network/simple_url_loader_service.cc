/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/services/network/simple_url_loader_service.h"

#include <algorithm>
#include <string>
#include <string_view>
#include <utility>

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/numerics/safe_conversions.h"
#include "base/task/thread_pool.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "net/base/net_errors.h"
#include "net/http/http_request_headers.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/mojom/url_response_head.mojom.h"
#include "url/gurl.h"

namespace brave::network {

namespace {

std::string_view MethodToString(mojom::HttpMethod method) {
  switch (method) {
    case mojom::HttpMethod::kGet:
      return net::HttpRequestHeaders::kGetMethod;
    case mojom::HttpMethod::kHead:
      return net::HttpRequestHeaders::kHeadMethod;
    case mojom::HttpMethod::kPost:
      return net::HttpRequestHeaders::kPostMethod;
    case mojom::HttpMethod::kPut:
      return net::HttpRequestHeaders::kPutMethod;
    case mojom::HttpMethod::kPatch:
      return net::HttpRequestHeaders::kPatchMethod;
    case mojom::HttpMethod::kDelete:
      return net::HttpRequestHeaders::kDeleteMethod;
  }
}

std::string_view AsStringView(const std::vector<uint8_t>& bytes) {
  return std::string_view(reinterpret_cast<const char*>(bytes.data()),
                          bytes.size());
}

std::vector<uint8_t> ToBytes(std::string_view text) {
  return std::vector<uint8_t>(text.begin(), text.end());
}

int RetryModeFrom(const mojom::RetryOptions& options) {
  int mode = ::network::SimpleURLLoader::RETRY_NEVER;
  if (options.retry_on_5xx) {
    mode |= ::network::SimpleURLLoader::RETRY_ON_5XX;
  }
  if (options.retry_on_network_change) {
    mode |= ::network::SimpleURLLoader::RETRY_ON_NETWORK_CHANGE;
  }
  if (options.retry_on_name_not_resolved) {
    mode |= ::network::SimpleURLLoader::RETRY_ON_NAME_NOT_RESOLVED;
  }
  return mode;
}

std::vector<mojom::HttpHeaderPtr> CollectResponseHeaders(
    const net::HttpResponseHeaders* headers) {
  std::vector<mojom::HttpHeaderPtr> result;
  if (!headers) {
    return result;
  }

  size_t iter = 0;
  std::string name;
  std::string value;
  while (headers->EnumerateHeaderLines(&iter, &name, &value)) {
    auto header = mojom::HttpHeader::New();
    header->name = name;
    // Header values are arbitrary octets, which is why the Mojom carries them
    // as array<uint8> rather than string. Sending a non-UTF-8 Mojom string
    // fails validation and terminates the sending process, so this must not
    // become a string without sanitizing first.
    header->value = ToBytes(value);
    result.push_back(std::move(header));
  }
  return result;
}

mojom::DownloadResultPtr MakeErrorResult(int net_error) {
  auto result = mojom::DownloadResult::New();
  result->net_error = net_error;
  result->response_code = -1;
  return result;
}

// Parses `body` as strict RFC 8259 JSON and re-serializes it canonically.
// Returns an empty string if it does not parse, or if the top level is neither
// an object nor an array.
//
// This intentionally matches //brave/components/api_request_helper, whose
// callers rely on a malformed body arriving as an empty one. Runs on a thread
// pool sequence because parsing untrusted input can be slow.
std::string SanitizeJson(std::string body) {
  auto parsed =
      base::JSONReader::ReadAndReturnValueWithError(body, base::JSON_PARSE_RFC);
  if (!parsed.has_value()) {
    return std::string();
  }
  if (!parsed->is_dict() && !parsed->is_list()) {
    return std::string();
  }

  std::string serialized;
  if (!base::JSONWriter::Write(*parsed, &serialized)) {
    return std::string();
  }
  return serialized;
}

}  // namespace

// One in-flight request.
//
// Owns the network::SimpleURLLoader, so destroying this cancels the load. Also
// serves the caller's DownloadHandle: if the caller closes its remote end, the
// receiver disconnects and the request is cancelled.
class SimpleUrlLoaderService::InFlightRequest : public mojom::DownloadHandle {
 public:
  InFlightRequest(std::unique_ptr<::network::SimpleURLLoader> loader,
                  mojo::PendingReceiver<mojom::DownloadHandle> cancellation,
                  bool sanitize_json_response,
                  scoped_refptr<base::SequencedTaskRunner> json_task_runner,
                  DownloadCallback callback,
                  base::OnceCallback<void(InFlightRequest*)> on_finished)
      : loader_(std::move(loader)),
        handle_receiver_(this, std::move(cancellation)),
        sanitize_json_response_(sanitize_json_response),
        json_task_runner_(std::move(json_task_runner)),
        callback_(std::move(callback)),
        on_finished_(std::move(on_finished)) {
    handle_receiver_.set_disconnect_handler(base::BindOnce(
        &InFlightRequest::OnCancelled, weak_factory_.GetWeakPtr()));
  }

  InFlightRequest(const InFlightRequest&) = delete;
  InFlightRequest& operator=(const InFlightRequest&) = delete;

  ~InFlightRequest() override = default;

  void Start(::network::SharedURLLoaderFactory* factory,
             size_t max_response_bytes) {
    loader_->DownloadToString(factory,
                              base::BindOnce(&InFlightRequest::OnBodyReceived,
                                             weak_factory_.GetWeakPtr()),
                              max_response_bytes);
  }

 private:
  void OnCancelled() {
    // Answer rather than drop the reply: dropping a response callback would
    // close the shared SimpleUrlLoader pipe and abort unrelated requests.
    Finish(MakeErrorResult(net::ERR_ABORTED));
  }

  void OnBodyReceived(std::optional<std::string> body) {
    auto result = mojom::DownloadResult::New();
    result->net_error = loader_->NetError();
    result->response_code = -1;
    result->final_url = loader_->GetFinalURL().spec();

    const auto* response_info = loader_->ResponseInfo();
    if (response_info && response_info->headers) {
      result->response_code = response_info->headers->response_code();
      result->headers = CollectResponseHeaders(response_info->headers.get());
    }

    if (!body) {
      Finish(std::move(result));
      return;
    }

    if (!sanitize_json_response_) {
      result->body = ToBytes(*body);
      Finish(std::move(result));
      return;
    }

    json_task_runner_->PostTaskAndReplyWithResult(
        FROM_HERE, base::BindOnce(&SanitizeJson, std::move(*body)),
        base::BindOnce(&InFlightRequest::OnJsonSanitized,
                       weak_factory_.GetWeakPtr(), std::move(result)));
  }

  void OnJsonSanitized(mojom::DownloadResultPtr result, std::string body) {
    result->body = ToBytes(body);
    Finish(std::move(result));
  }

  // Runs the reply and then destroys `this`. Must be the last thing any caller
  // does.
  void Finish(mojom::DownloadResultPtr result) {
    std::move(callback_).Run(std::move(result));
    std::move(on_finished_).Run(this);
  }

  std::unique_ptr<::network::SimpleURLLoader> loader_;
  mojo::Receiver<mojom::DownloadHandle> handle_receiver_;
  const bool sanitize_json_response_;
  scoped_refptr<base::SequencedTaskRunner> json_task_runner_;
  DownloadCallback callback_;
  base::OnceCallback<void(InFlightRequest*)> on_finished_;

  base::WeakPtrFactory<InFlightRequest> weak_factory_{this};
};

SimpleUrlLoaderService::SimpleUrlLoaderService(
    scoped_refptr<::network::SharedURLLoaderFactory> url_loader_factory,
    net::NetworkTrafficAnnotationTag traffic_annotation,
    scoped_refptr<base::SequencedTaskRunner> json_task_runner)
    : url_loader_factory_(std::move(url_loader_factory)),
      traffic_annotation_(traffic_annotation),
      json_task_runner_(json_task_runner
                            ? std::move(json_task_runner)
                            : base::ThreadPool::CreateSequencedTaskRunner(
                                  {base::TaskPriority::USER_VISIBLE})) {
  CHECK(url_loader_factory_);
}

SimpleUrlLoaderService::~SimpleUrlLoaderService() = default;

void SimpleUrlLoaderService::Bind(
    mojo::PendingReceiver<mojom::SimpleUrlLoader> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void SimpleUrlLoaderService::Download(
    mojom::DownloadRequestPtr request,
    mojo::PendingReceiver<mojom::DownloadHandle> cancellation,
    DownloadCallback callback) {
  GURL url(request->url);
  if (!url.is_valid() || !url.SchemeIsHTTPOrHTTPS()) {
    std::move(callback).Run(MakeErrorResult(net::ERR_INVALID_URL));
    return;
  }

  auto resource_request = std::make_unique<::network::ResourceRequest>();
  resource_request->url = std::move(url);
  resource_request->method = std::string(MethodToString(request->method));

  for (const auto& header : request->headers) {
    // SetHeader DCHECKs on malformed input, and these values crossed a process
    // boundary from a non-C++ caller, so validate rather than trust.
    std::string_view value = AsStringView(header->value);
    if (!net::HttpUtil::IsValidHeaderName(header->name) ||
        !net::HttpUtil::IsValidHeaderValue(value)) {
      std::move(callback).Run(MakeErrorResult(net::ERR_INVALID_ARGUMENT));
      return;
    }
    resource_request->headers.SetHeader(header->name, value);
  }

  const bool has_body = request->body.has_value();
  if (has_body != request->body_content_type.has_value()) {
    std::move(callback).Run(MakeErrorResult(net::ERR_INVALID_ARGUMENT));
    return;
  }

  auto loader = ::network::SimpleURLLoader::Create(std::move(resource_request),
                                                   traffic_annotation_);
  loader->SetAllowHttpErrorResults(request->allow_http_error_results);

  if (has_body) {
    loader->AttachStringForUpload(AsStringView(*request->body),
                                  *request->body_content_type);
  }

  if (request->retry_options) {
    const auto& retry = *request->retry_options;
    const int mode = RetryModeFrom(retry);
    // SimpleURLLoader requires max_retries == 0 when the mode is RETRY_NEVER.
    if (mode != ::network::SimpleURLLoader::RETRY_NEVER &&
        retry.max_retries > 0) {
      loader->SetRetryOptions(base::saturated_cast<int>(retry.max_retries),
                              mode);
    }
  }

  size_t max_response_bytes =
      ::network::SimpleURLLoader::kMaxBoundedStringDownloadSize;
  if (request->max_response_bytes > 0) {
    max_response_bytes = std::min(
        max_response_bytes, static_cast<size_t>(request->max_response_bytes));
  }

  auto in_flight = std::make_unique<InFlightRequest>(
      std::move(loader), std::move(cancellation),
      request->sanitize_json_response, json_task_runner_, std::move(callback),
      base::BindOnce(&SimpleUrlLoaderService::Finish, base::Unretained(this)));
  InFlightRequest* in_flight_ptr = in_flight.get();
  requests_.insert(std::move(in_flight));

  in_flight_ptr->Start(url_loader_factory_.get(), max_response_bytes);
}

void SimpleUrlLoaderService::Finish(InFlightRequest* request) {
  // base::UniquePtrComparator makes `find` heterogeneous, but `erase` still
  // needs an iterator.
  auto it = requests_.find(request);
  CHECK(it != requests_.end());
  requests_.erase(it);
}

}  // namespace brave::network
