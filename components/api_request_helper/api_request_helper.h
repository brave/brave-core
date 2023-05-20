/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_API_REQUEST_HELPER_API_REQUEST_HELPER_H_
#define BRAVE_COMPONENTS_API_REQUEST_HELPER_API_REQUEST_HELPER_H_

#include <list>
#include <memory>
#include <string>

#include "base/containers/flat_map.h"
#include "base/files/file_path.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/time/time.h"
#include "base/values.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "services/data_decoder/public/cpp/data_decoder.h"
#include "services/data_decoder/public/mojom/json_parser.mojom.h"
#include "services/network/public/cpp/simple_url_loader.h"
#include "services/network/public/cpp/simple_url_loader_stream_consumer.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace api_request_helper {

class APIRequestResult {
 public:
  APIRequestResult();
  APIRequestResult(int response_code,
                   std::string body,
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
  // Sanitized json response.
  const std::string& body() const { return body_; }
  // `base::Value` of sanitized json response.
  const base::Value& value_body() const { return value_body_; }
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
  int response_code_ = -1;
  std::string body_;
  base::Value value_body_;
  base::flat_map<std::string, std::string> headers_;
  int error_code_ = -1;
  GURL final_url_;
};

struct APIRequestOptions {
  bool auto_retry_on_network_change = false;
  bool enable_cache = false;
  size_t max_body_size = -1u;
  absl::optional<base::TimeDelta> timeout;
};

// Anyone is welcome to use APIRequestHelper to reduce boilerplate
class APIRequestHelper {
 public:
  using DataReceivedCallback =
      base::RepeatingCallback<void(const std::string&)>;
  using DataCompletedCallback =
      base::OnceCallback<void(bool success, int response_code)>;

  class LoaderWrapperHandler : public network::SimpleURLLoaderStreamConsumer {
   public:
    explicit LoaderWrapperHandler(APIRequestHelper* api_request_helper);
    ~LoaderWrapperHandler() override;
    LoaderWrapperHandler(const LoaderWrapperHandler&) = delete;
    LoaderWrapperHandler& operator=(const LoaderWrapperHandler&) = delete;

    // network::SimpleURLLoaderStreamConsumer implementation:
    void OnDataReceived(base::StringPiece string_piece,
                        base::OnceClosure resume) override;
    void OnComplete(bool success) override;
    void OnRetry(base::OnceClosure start_retry) override;

    void OnParseJsonFromStream(data_decoder::DataDecoder::ValueOrError result);
    void RegisterURLLoader(std::unique_ptr<network::SimpleURLLoader> loader);

   private:
    friend class APIRequestHelper;
    std::unique_ptr<network::SimpleURLLoader> url_loader_;
    raw_ptr<APIRequestHelper> api_request_helper_;

    // Streaming callbacks
    DataReceivedCallback data_received_callback_;
    DataCompletedCallback data_completed_callback_;

    base::WeakPtrFactory<LoaderWrapperHandler> weak_ptr_factory_{this};
  };

  using Ticket = std::list<std::unique_ptr<LoaderWrapperHandler>>::iterator;

  using ResultCallback = base::OnceCallback<void(APIRequestResult)>;
  using ResponseConversionCallback =
      base::OnceCallback<absl::optional<std::string>(
          const std::string& raw_response)>;

  APIRequestHelper(
      net::NetworkTrafficAnnotationTag annotation_tag,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~APIRequestHelper();

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

  Ticket RequestSSE(
      const std::string& method,
      const GURL& url,
      const std::string& payload,
      const std::string& payload_content_type,
      DataReceivedCallback data_received_callback,
      DataCompletedCallback data_completed_callback,
      const base::flat_map<std::string, std::string>& headers = {},
      const APIRequestOptions& request_options = {});

  using DownloadCallback = base::OnceCallback<void(
      base::FilePath,
      const base::flat_map<std::string, std::string>& /*response_headers*/)>;
  Ticket Download(const GURL& url,
                  const std::string& payload,
                  const std::string& payload_content_type,
                  const base::FilePath& path,
                  DownloadCallback callback,
                  const base::flat_map<std::string, std::string>& headers = {},
                  const APIRequestOptions& request_options = {});

  void Cancel(const Ticket& ticket);
  void Cancel(LoaderWrapperHandler* request_handler);

 private:
  APIRequestHelper(const APIRequestHelper&) = delete;
  APIRequestHelper& operator=(const APIRequestHelper&) = delete;

  std::unique_ptr<LoaderWrapperHandler> CreateLoaderWrapperHandler(
      const std::string& method,
      const GURL& url,
      const std::string& payload,
      const std::string& payload_content_type,
      bool auto_retry_on_network_change,
      bool enable_cache,
      bool allow_http_error_result,
      const base::flat_map<std::string, std::string>& headers);

  using LoaderWrapperHandlerList =
      std::list<std::unique_ptr<LoaderWrapperHandler>>;
  void OnResponse(Ticket iter,
                  ResultCallback callback,
                  ResponseConversionCallback conversion_callback,
                  const std::unique_ptr<std::string> response_body);
  void OnDownload(Ticket iter, DownloadCallback callback, base::FilePath path);

  // To ensure ordered processing of stream chunks, we create our own instances
  // of DataDecoder and remote for JsonParser. This avoids the issue of
  // unordered chunks that can occur when calling the static function, which
  // creates a new instance of the process for each call. By using a single
  // instance of the parser, we can reuse it for consecutive calls.
  std::unique_ptr<data_decoder::DataDecoder> data_decoder_;
  mojo::Remote<data_decoder::mojom::JsonParser> json_parser_;

  net::NetworkTrafficAnnotationTag annotation_tag_;
  LoaderWrapperHandlerList url_loaders_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::WeakPtrFactory<APIRequestHelper> weak_ptr_factory_{this};
};

}  // namespace api_request_helper

#endif  // BRAVE_COMPONENTS_API_REQUEST_HELPER_API_REQUEST_HELPER_H_
