/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_API_REQUEST_HELPER_API_REQUEST_HELPER_H_
#define BRAVE_COMPONENTS_API_REQUEST_HELPER_API_REQUEST_HELPER_H_

#include <list>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/functional/callback_forward.h"
#include "base/functional/callback_helpers.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/cpp/simple_url_loader_stream_consumer.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace data_decoder {
class DataDecoder;
}

namespace api_request_helper {

class APIRequestResult {
 public:
  APIRequestResult();
  APIRequestResult(int response_code,
                   base::Value value_body,
                   base::flat_map<std::string, std::string> headers,
                   int error_code,
                   GURL final_url);
  APIRequestResult(const APIRequestResult&) = delete;
  APIRequestResult& operator=(const APIRequestResult&) = delete;
  APIRequestResult(APIRequestResult&&);
  APIRequestResult& operator=(APIRequestResult&&);
  ~APIRequestResult();

  bool operator==(const APIRequestResult& other) const;
  bool operator!=(const APIRequestResult& other) const;

  bool Is2XXResponseCode() const;
  bool IsResponseCodeValid() const;

  // HTTP response code.
  int response_code() const { return response_code_; }

  // Extract the sanitized response as base::Value.
  base::Value TakeBody();

  // Returns the sanitized response as base::Value.
  // Note: don't clone large responses, use TakeBody() instead.
  const base::Value& value_body() const { return value_body_; }

  // Serialize the sanitized response and returns it as string.
  // Note: use TakeBody()/value_body() instead where possible.
  std::string SerializeBodyToString() const;

  // HTTP response headers.
  const base::flat_map<std::string, std::string>& headers() const {
    return headers_;
  }
  // `net::Error` code
  int error_code() const { return error_code_; }
  // Actual url requested. May differ from original request url in case of
  // redirects happened.
  GURL final_url() const { return final_url_; }

 private:
  friend class APIRequestHelper;

  int response_code_ = -1;
  base::Value value_body_;
  base::flat_map<std::string, std::string> headers_;
  int error_code_ = -1;
  GURL final_url_;
  bool body_consumed_ = false;
};

struct APIRequestOptions {
  bool auto_retry_on_network_change = false;
  bool enable_cache = false;
  size_t max_body_size = -1u;
  std::optional<base::TimeDelta> timeout;
};

using ValueOrError = base::expected<base::Value, std::string>;

// Anyone is welcome to use APIRequestHelper to reduce boilerplate
// Unit tests which need to use the data decoding from this class can use
// data_decoder::test::InProcessDataDecoder to run all decode operations
// in-process.
class APIRequestHelper {
 public:
  using DataReceivedCallback =
      base::RepeatingCallback<void(ValueOrError result)>;
  using ResultCallback = base::OnceCallback<void(APIRequestResult)>;
  using ResponseStartedCallback =
      base::OnceCallback<void(const std::string& url,
                              const int64_t content_length)>;
  using ResponseConversionCallback =
      base::OnceCallback<std::optional<std::string>(
          const std::string& raw_response)>;

  class URLLoaderHandler : public network::SimpleURLLoaderStreamConsumer {
   public:
    URLLoaderHandler(APIRequestHelper* api_request_helper,
                     scoped_refptr<base::SequencedTaskRunner> task_runner);
    ~URLLoaderHandler() override;
    URLLoaderHandler(const URLLoaderHandler&) = delete;
    URLLoaderHandler& operator=(const URLLoaderHandler&) = delete;

    void RegisterURLLoader(std::unique_ptr<network::SimpleURLLoader> loader);
    void SetResultCallback(ResultCallback result_callback);
    base::WeakPtr<URLLoaderHandler> GetWeakPtr();

    void send_sse_data_for_testing(std::string_view string_piece,
                                   bool is_sse,
                                   DataReceivedCallback callback);

   private:
    friend class APIRequestHelper;

    void ParseJsonImpl(std::string json,
                       base::OnceCallback<void(ValueOrError)> callback);

    // Run completion callback if there are no operations in progress.
    // If Cancel is needed even if url or data operations are in progress,
    // then call |APIRequestHelper::Cancel|.
    void MaybeSendResult();
    void ParseSSE(std::string_view string_piece);

    // network::SimpleURLLoaderStreamConsumer implementation:
    void OnDataReceived(std::string_view string_piece,
                        base::OnceClosure resume) override;
    void OnComplete(bool success) override;
    void OnRetry(base::OnceClosure start_retry) override;

    // This is used for one shot responses
    void OnResponse(ResponseConversionCallback conversion_callback,
                    const std::unique_ptr<std::string> response_body);

    // Decode one shot responses
    void OnParseJsonResponse(APIRequestResult result,
                             ValueOrError result_value);

    std::unique_ptr<network::SimpleURLLoader> url_loader_;
    raw_ptr<APIRequestHelper> api_request_helper_;

    DataReceivedCallback data_received_callback_;
    ResponseStartedCallback response_started_callback_;
    ResultCallback result_callback_;
    ResponseConversionCallback conversion_callback_;

    bool is_sse_ = false;

    // To ensure ordered processing of stream chunks, we create our own
    // instance of DataDecoder per request. This avoids the issue
    // of unordered chunks that can occur when calling the static function,
    // which creates a new instance of the process for each call. By using a
    // single instance of the parser, we can reuse it for consecutive calls.
    std::unique_ptr<data_decoder::DataDecoder> data_decoder_;
    // Keep track of number of in-progress data decoding operations
    // so that we can know if any are still in-progress when the request
    // completes.
    int current_decoding_operation_count_ = 0;
    bool request_is_finished_ = false;

    const scoped_refptr<base::SequencedTaskRunner> task_runner_;

    base::WeakPtrFactory<URLLoaderHandler> weak_ptr_factory_{this};
  };

  using URLLoaderHandlerList = std::list<std::unique_ptr<URLLoaderHandler>>;
  using Ticket = std::list<std::unique_ptr<URLLoaderHandler>>::iterator;

  APIRequestHelper(
      net::NetworkTrafficAnnotationTag annotation_tag,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  virtual ~APIRequestHelper();

  // Each response is expected in json format and will be validated through
  // JsonSanitizer. In cases where json contains values that are not supported
  // by the standard base/json parser it is necessary to convert such values
  // into string before validating the response. For these purposes
  // conversion_callback is added which receives raw response and can perform
  // necessary conversions.
  Ticket Request(
      const std::string& method,
      const GURL& url,
      const std::string& payload,
      const std::string& payload_content_type,
      ResultCallback callback,
      const base::flat_map<std::string, std::string>& headers = {},
      const APIRequestOptions& request_options = {},
      ResponseConversionCallback conversion_callback = base::NullCallback());

  // TODO(petemill): Avoid needing this to be virtual only for mocking during
  // testing by using an interface, delegates, intercepting streaming, or
  // another pattern.
  virtual Ticket RequestSSE(
      const std::string& method,
      const GURL& url,
      const std::string& payload,
      const std::string& payload_content_type,
      DataReceivedCallback data_received_callback,
      ResultCallback result_callback,
      const base::flat_map<std::string, std::string>& headers,
      const APIRequestOptions& request_options);

  virtual Ticket RequestSSE(
      const std::string& method,
      const GURL& url,
      const std::string& payload,
      const std::string& payload_content_type,
      DataReceivedCallback data_received_callback,
      ResultCallback result_callback,
      const base::flat_map<std::string, std::string>& headers,
      const APIRequestOptions& request_options,
      ResponseStartedCallback response_started_callback);

  void Cancel(const Ticket& ticket);
  void CancelAll();

  void SetUrlLoaderFactoryForTesting(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);

 private:
  APIRequestHelper(const APIRequestHelper&) = delete;
  APIRequestHelper& operator=(const APIRequestHelper&) = delete;

  APIRequestHelper::Ticket CreateURLLoaderHandler(
      const std::string& method,
      const GURL& url,
      const std::string& payload,
      const std::string& payload_content_type,
      bool auto_retry_on_network_change,
      bool enable_cache,
      bool allow_http_error_result,
      const base::flat_map<std::string, std::string>& headers);

  // TODO(petemill): When Download has been removed, we don't need two versions
  // of CreateURLLoaderHandler
  APIRequestHelper::Ticket CreateRequestURLLoaderHandler(
      const std::string& method,
      const GURL& url,
      const std::string& payload,
      const std::string& payload_content_type,
      const APIRequestOptions& request_options,
      const base::flat_map<std::string, std::string>& headers,
      ResultCallback result_callback);

  void DeleteAndSendResult(Ticket iter,
                           ResultCallback callback,
                           APIRequestResult result);

  net::NetworkTrafficAnnotationTag annotation_tag_;
  URLLoaderHandlerList url_loaders_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  const scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::WeakPtrFactory<APIRequestHelper> weak_ptr_factory_{this};
};

void ParseJsonNonBlocking(std::string json,
                          base::OnceCallback<void(ValueOrError)> callback);

}  // namespace api_request_helper

#endif  // BRAVE_COMPONENTS_API_REQUEST_HELPER_API_REQUEST_HELPER_H_
