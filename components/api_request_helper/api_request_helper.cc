/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/api_request_helper/api_request_helper.h"

#include <string_view>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/check_op.h"
#include "base/debug/alias.h"
#include "base/debug/dump_without_crashing.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/memory/raw_ptr.h"
#include "base/metrics/histogram_functions.h"
#include "base/ranges/algorithm.h"
#include "base/strings/string_split.h"
#include "base/task/thread_pool.h"
#include "base/time/time.h"
#include "base/timer/elapsed_timer.h"
#include "base/trace_event/trace_event.h"
#include "net/base/load_flags.h"
#include "net/http/http_status_code.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"
#include "services/network/public/mojom/url_response_head.mojom.h"

namespace api_request_helper {

namespace {

const unsigned int kRetriesCountOnNetworkChange = 1;

void ParseJsonInWorkerTaskRunner(
    std::string json,
    base::SequencedTaskRunner* task_runner,
    data_decoder::DataDecoder::ValueParseCallback callback) {
  task_runner->PostTaskAndReplyWithResult(
      FROM_HERE,
      base::BindOnce(
          [](std::string json) {
            return base::JSONReader::ReadAndReturnValueWithError(
                json, base::JSON_PARSE_RFC);
          },
          std::move(json)),
      base::BindOnce(
          [](data_decoder::DataDecoder::ValueParseCallback callback,
             base::JSONReader::Result result) {
            if (!result.has_value()) {
              std::move(callback).Run(base::unexpected(result.error().message));
            } else {
              std::move(callback).Run(std::move(*result));
            }
          },
          std::move(callback)));
}

scoped_refptr<base::SequencedTaskRunner> MakeDecoderTaskRunner() {
  return base::ThreadPool::CreateSequencedTaskRunner(
      {base::TaskPriority::USER_VISIBLE,
       base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN});
}

APIRequestResult ToAPIRequestResult(
    std::unique_ptr<network::SimpleURLLoader> loader) {
  auto response_code = -1;
  auto error_code = loader->NetError();
  auto final_url = loader->GetFinalURL();
  base::flat_map<std::string, std::string> headers;
  if (loader->ResponseInfo()) {
    auto headers_list = loader->ResponseInfo()->headers;
    if (headers_list) {
      response_code = headers_list->response_code();
      DVLOG(1) << "Response code: " << response_code;
      size_t header_iter = 0;
      std::string key;
      std::string value;
      while (headers_list->EnumerateHeaderLines(&header_iter, &key, &value)) {
        key = base::ToLowerASCII(key);
        headers[key] = value;
        DVLOG(2) << "< " << key << ": " << value;
      }
    }
  }

  return APIRequestResult(response_code, base::Value(), std::move(headers),
                          error_code, final_url);
}

// A helper class to measure performance of the callback.
class ScopedPerfTracker {
 public:
  explicit ScopedPerfTracker(const char* uma_name) : uma_name_(uma_name) {}

  ~ScopedPerfTracker() {
    if (timer_.is_supported()) {
      base::UmaHistogramMediumTimes(uma_name_, timer_.Elapsed());
    }
  }

 private:
  raw_ptr<const char> uma_name_;
  base::ElapsedThreadTimer timer_;
};

}  // namespace

APIRequestResult::APIRequestResult() = default;
APIRequestResult::APIRequestResult(
    int response_code,
    base::Value value_body,
    base::flat_map<std::string, std::string> headers,
    int error_code,
    GURL final_url)
    : response_code_(response_code),
      value_body_(std::move(value_body)),
      headers_(std::move(headers)),
      error_code_(error_code),
      final_url_(std::move(final_url)) {}
APIRequestResult::APIRequestResult(APIRequestResult&&) = default;
APIRequestResult& APIRequestResult::operator=(APIRequestResult&&) = default;
APIRequestResult::~APIRequestResult() = default;

bool APIRequestResult::operator==(const APIRequestResult& other) const {
  auto tied = [](auto& v) {
    return std::tie(v.response_code_, v.value_body_, v.headers_, v.error_code_,
                    v.final_url_);
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

base::Value APIRequestResult::TakeBody() {
  CHECK(!body_consumed_);
  body_consumed_ = true;
  return std::move(value_body_);
}

std::string APIRequestResult::SerializeBodyToString() const {
  if (value_body_.is_none()) {
    return std::string();
  }
  std::string safe_json;
  if (!base::JSONWriter::Write(value_body_, &safe_json)) {
    VLOG(1) << "Response validation error: Encoding error";
  }

  return safe_json;
}

APIRequestHelper::APIRequestHelper(
    net::NetworkTrafficAnnotationTag annotation_tag,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : annotation_tag_(annotation_tag),
      url_loader_factory_(url_loader_factory),
      task_runner_(MakeDecoderTaskRunner()) {}

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
  auto iter = CreateRequestURLLoaderHandler(
      method, url, payload, payload_content_type, request_options, headers,
      std::move(callback));
  auto* handler = iter->get();

  if (request_options.max_body_size == -1u) {
    handler->url_loader_->DownloadToStringOfUnboundedSizeUntilCrashAndDie(
        url_loader_factory_.get(),
        base::BindOnce(&APIRequestHelper::URLLoaderHandler::OnResponse,
                       handler->GetWeakPtr(), std::move(conversion_callback)));
  } else {
    handler->url_loader_->DownloadToString(
        url_loader_factory_.get(),
        base::BindOnce(&APIRequestHelper::URLLoaderHandler::OnResponse,
                       handler->GetWeakPtr(), std::move(conversion_callback)),
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
    ResultCallback result_callback,
    const base::flat_map<std::string, std::string>& headers,
    const APIRequestOptions& request_options) {
  return RequestSSE(method, url, payload, payload_content_type,
                    std::move(data_received_callback),
                    std::move(result_callback), headers, request_options,
                    base::NullCallback());
}

APIRequestHelper::Ticket APIRequestHelper::RequestSSE(
    const std::string& method,
    const GURL& url,
    const std::string& payload,
    const std::string& payload_content_type,
    DataReceivedCallback data_received_callback,
    ResultCallback result_callback,
    const base::flat_map<std::string, std::string>& headers,
    const APIRequestOptions& request_options,
    ResponseStartedCallback response_started_callback) {
  auto iter = CreateRequestURLLoaderHandler(
      method, url, payload, payload_content_type, request_options, headers,
      std::move(result_callback));
  auto* handler = iter->get();

  // Set streaming data callback
  handler->data_received_callback_ = std::move(data_received_callback);

  handler->response_started_callback_ = std::move(response_started_callback);

  handler->url_loader_->DownloadAsStream(url_loader_factory_.get(), handler);
  return iter;
}

void APIRequestHelper::DeleteAndSendResult(Ticket iter,
                                           ResultCallback callback,
                                           APIRequestResult result) {
  Cancel(iter);
  std::move(callback).Run(std::move(result));
}

void APIRequestHelper::Cancel(const Ticket& ticket) {
  url_loaders_.erase(ticket);
}

void APIRequestHelper::CancelAll() {
  url_loaders_.clear();
}

APIRequestHelper::Ticket APIRequestHelper::CreateURLLoaderHandler(
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
  if (!method.empty()) {
    request->method = method;
  }

  DVLOG(4) << method << " " << url.spec();

  if (!headers.empty()) {
    for (auto entry : headers) {
      DVLOG(4) << "> " << entry.first << ": " << entry.second;
      request->headers.SetHeader(entry.first, entry.second);
    }
  }

  if (!payload.empty()) {
    DVLOG(4) << "Payload type " << payload_content_type << ":";
    DVLOG(4) << payload;
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

  auto loader_wrapper_handler =
      std::make_unique<URLLoaderHandler>(this, task_runner_);
  loader_wrapper_handler->RegisterURLLoader(std::move(url_loader));

  auto iter = url_loaders_.insert(url_loaders_.begin(),
                                  std::move(loader_wrapper_handler));

  return iter;
}

APIRequestHelper::Ticket APIRequestHelper::CreateRequestURLLoaderHandler(
    const std::string& method,
    const GURL& url,
    const std::string& payload,
    const std::string& payload_content_type,
    const APIRequestOptions& request_options,
    const base::flat_map<std::string, std::string>& headers,
    ResultCallback result_callback) {
  auto iter = CreateURLLoaderHandler(
      method, url, payload, payload_content_type,
      request_options.auto_retry_on_network_change,
      request_options.enable_cache, true /* allow_http_error_result*/, headers);
  auto* handler = iter->get();

  handler->result_callback_ = base::BindOnce(
      &APIRequestHelper::DeleteAndSendResult, weak_ptr_factory_.GetWeakPtr(),
      iter, std::move(result_callback));
  if (request_options.timeout) {
    handler->url_loader_->SetTimeoutDuration(request_options.timeout.value());
  }
  return iter;
}

APIRequestHelper::URLLoaderHandler::URLLoaderHandler(
    APIRequestHelper* api_request_helper,
    scoped_refptr<base::SequencedTaskRunner> task_runner)
    : api_request_helper_(api_request_helper),
      task_runner_(std::move(task_runner)) {}
APIRequestHelper::URLLoaderHandler::~URLLoaderHandler() = default;

void APIRequestHelper::URLLoaderHandler::RegisterURLLoader(
    std::unique_ptr<network::SimpleURLLoader> loader) {
  url_loader_ = std::move(loader);

  auto on_response_start =
      [](base::WeakPtr<APIRequestHelper::URLLoaderHandler> handler,
         const GURL& final_url,
         const network::mojom::URLResponseHead& response_head) {
        if (handler) {
          if (response_head.mime_type == "text/event-stream") {
            handler->is_sse_ = true;
          }
          if (handler->response_started_callback_) {
            std::move(handler->response_started_callback_)
                .Run(final_url.spec(), response_head.content_length);
          }
        }
      };

  url_loader_->SetOnResponseStartedCallback(base::BindOnce(
      std::move(on_response_start), weak_ptr_factory_.GetWeakPtr()));
}

base::WeakPtr<APIRequestHelper::URLLoaderHandler>
APIRequestHelper::URLLoaderHandler::GetWeakPtr() {
  return weak_ptr_factory_.GetWeakPtr();
}

void APIRequestHelper::URLLoaderHandler::send_sse_data_for_testing(
    std::string_view string_piece,
    bool is_sse,
    DataReceivedCallback callback) {
  is_sse_ = is_sse;
  data_received_callback_ = std::move(callback);
  OnDataReceived(string_piece, base::BindOnce([]() {}));
}

void APIRequestHelper::URLLoaderHandler::ParseJsonImpl(
    std::string json,
    base::OnceCallback<void(ValueOrError)> callback) {
  if (base::JSONReader::UsingRust()) {
    ParseJsonInWorkerTaskRunner(std::move(json), task_runner_.get(),
                                std::move(callback));
    return;
  }

  if (!data_decoder_) {
    VLOG(1) << "Creating DataDecoder for APIRequestHelper";
    data_decoder_ = std::make_unique<data_decoder::DataDecoder>();
  }

  data_decoder_->ParseJson(json, std::move(callback));
}

void APIRequestHelper::URLLoaderHandler::OnDataReceived(
    std::string_view string_piece,
    base::OnceClosure resume) {
  DVLOG(2) << "[[" << __func__ << "]]" << " Chunk received";
  if (is_sse_) {
    ParseSSE(string_piece);
  } else {
    DVLOG(4) << "Chunk content: \n" << string_piece;
    TRACE_EVENT0("brave", "APIRequestHelper_OnDataReceivedNoSSE");
    ScopedPerfTracker tracker("Brave.APIRequestHelper.OnDataReceivedNoSSE");
    data_received_callback_.Run(base::Value(string_piece));
  }
  // Get next chunk
  // TODO(petemill): Consider waiting until Parsing finishes to resume,
  // then we don't need to worry about decoding order or counting decoding
  // operations. Perhaps also provide the |resume| closure to the consumer
  // so that we can be notified when ready for the next chunk and not overwhelm
  // e.g. the UI.
  std::move(resume).Run();
}

void APIRequestHelper::URLLoaderHandler::OnComplete(bool success) {
  DCHECK(result_callback_);
  VLOG(1) << "[[" << __func__ << "]]"
          << " Response completed\n";

  request_is_finished_ = true;

  // Delete now or when decoding operations are complete
  MaybeSendResult();
}

void APIRequestHelper::URLLoaderHandler::OnRetry(
    base::OnceClosure start_retry) {
  std::move(start_retry).Run();
  // We assume that a consumer of APIRequestHelper doesn't
  // care about discarding partial responses received so far
  // before a retry, especially if it's SSE. If this assumption
  // becomes incorrect, perhaps that caller should make the request
  // directly, or APIRequestHelper could accept a callback, or move
  // to an observer model.
}

void APIRequestHelper::URLLoaderHandler::OnResponse(
    ResponseConversionCallback conversion_callback,
    const std::unique_ptr<std::string> response_body) {
  VLOG(1) << "[[" << __func__ << "]]"
          << " Response received\n";
  DCHECK(result_callback_);
  // This shouldn't be called on a request with multiple decoding operations,
  // otherwise we need to modify this to use data_chunk_parsed_callback_.
  DCHECK_EQ(current_decoding_operation_count_, 0);
  APIRequestResult result = ToAPIRequestResult(std::move(url_loader_));

  if (!response_body) {
    std::move(result_callback_).Run(std::move(result));
    return;
  }
  auto& raw_body = *response_body;
  if (conversion_callback) {
    auto converted_body = std::move(conversion_callback).Run(raw_body);
    if (!converted_body) {
      result.response_code_ = 422;
      std::move(result_callback_).Run(std::move(result));
      return;
    }
    raw_body = converted_body.value();
  }

  ParseJsonImpl(
      std::move(raw_body),
      base::BindOnce(&APIRequestHelper::URLLoaderHandler::OnParseJsonResponse,
                     GetWeakPtr(), std::move(result)));
}

void APIRequestHelper::URLLoaderHandler::OnParseJsonResponse(
    APIRequestResult result,
    ValueOrError result_value) {
  TRACE_EVENT1("brave", "APIRequestHelper_ProcessResultOnUI", "url",
               result.final_url().spec());
  ScopedPerfTracker tracker("Brave.APIRequestHelper.ProcessResultOnUI");
  // TODO(petemill): Simplify by combining OnParseJsonResponse with the Json
  // response handler in ParseSSE.
  if (!result_value.has_value()) {
    VLOG(1) << "Response validation error:" << result_value.error();
    if (result_value.error().starts_with("trailing comma")) {
      // Rust parser returns the trailing comma error. Log the
      // urls and send a crash dump to find where trailing commas
      // could still be used.
      DEBUG_ALIAS_FOR_GURL(url_alias, result.final_url());
      DEBUG_ALIAS_FOR_CSTR(result_str, result_value.error().c_str(), 1024);
      base::debug::DumpWithoutCrashing();
    }
    std::move(result_callback_).Run(std::move(result));
    return;
  }
  if (!result_value.value().is_dict() && !result_value.value().is_list()) {
    VLOG(1) << "Response validation error: Invalid top-level type";
    std::move(result_callback_).Run(std::move(result));
    return;
  }

  VLOG(2) << "Response validation successful";
  result.value_body_ = std::move(result_value.value());
  std::move(result_callback_).Run(std::move(result));
}

void APIRequestHelper::URLLoaderHandler::MaybeSendResult() {
  // Verify that counting hasn't gone wrong - it should never be less than 0.
  DCHECK_LE(0, current_decoding_operation_count_);
  // Don't allow completion if decoding is still in progress so that
  // we don't delete the reference to |data_decoder_| which would cancel the
  // operations.
  // Response must be complete to know that decoding is definitely complete
  // since streaming responses may get more reponse chunks which need decoding.
  const bool decoding_is_complete = (current_decoding_operation_count_ == 0);

  if (request_is_finished_ && decoding_is_complete) {
    std::move(result_callback_).Run(ToAPIRequestResult(std::move(url_loader_)));
  } else if (decoding_is_complete) {
    VLOG(3) << "Did not run URLLoaderHandler completion handler, still have "
            << current_decoding_operation_count_
            << " decoding operations in progress, waiting for them to"
            << " complete...";
  }
}

void APIRequestHelper::URLLoaderHandler::ParseSSE(
    std::string_view string_piece) {
  // New chunks should only be received before the request is completed
  DCHECK(!request_is_finished_);
  // We split the string into multiple chunks because there are cases where
  // multiple chunks are received in a single call.
  std::vector<std::string_view> stream_data = base::SplitStringPiece(
      string_piece, "\r\n", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  // Remove SSE events that don't look like JSON - could be string or [DONE]
  // message.
  // TODO(@nullhook): Parse both JSON and string values. The below currently
  // only identifies JSON values.
  static constexpr char kDataPrefix[] = "data: {";
  std::erase_if(stream_data, [](std::string_view item) {
    DVLOG(3) << "Received chunk: " << item;
    if (!item.starts_with(kDataPrefix)) {
      // This is useful to log in case an API starts
      // coming back with unknown data type in some
      // scenarios.
      VLOG(1) << "Data did not start with SSE prefix";
      return true;
    }
    return false;
  });

  // Keep track of number of in-progress data decoding operations
  // so that we can know if any are still in-progress when the request
  // completes.
  current_decoding_operation_count_ += stream_data.size();

  for (const auto& data : stream_data) {
    auto json = data.substr(strlen(kDataPrefix) - 1);
    auto on_json_parsed =
        [](base::WeakPtr<APIRequestHelper::URLLoaderHandler> handler,
           ValueOrError result) {
          DVLOG(2) << "Chunk parsed";
          if (!handler) {
            return;
          }
          TRACE_EVENT0("brave", "APIRequestHelper_ParseSSECallback");
          ScopedPerfTracker tracker("Brave.APIRequestHelper.ParseSSECallback");
          handler->current_decoding_operation_count_--;
          DCHECK(handler->data_received_callback_);
          handler->data_received_callback_.Run(std::move(result));
          // Parsing is potentially the last operation for |URLLoaderHandler|.
          handler->MaybeSendResult();
        };

    DVLOG(2) << "Going to call ParseJsonImpl";
    ParseJsonImpl(std::string(json),
                  base::BindOnce(std::move(on_json_parsed),
                                 weak_ptr_factory_.GetWeakPtr()));
  }
}

void APIRequestHelper::SetUrlLoaderFactoryForTesting(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory) {
  url_loader_factory_ = std::move(url_loader_factory);
}

void ParseJsonNonBlocking(std::string json,
                          base::OnceCallback<void(ValueOrError)> callback) {
  if (base::JSONReader::UsingRust()) {
    auto task_runner = MakeDecoderTaskRunner();
    ParseJsonInWorkerTaskRunner(std::move(json), task_runner.get(),
                                std::move(callback));
    return;
  }

  data_decoder::DataDecoder::ParseJsonIsolated(json, std::move(callback));
}

}  // namespace api_request_helper
