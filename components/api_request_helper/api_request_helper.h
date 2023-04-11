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
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
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
  using Ticket = std::list<std::unique_ptr<network::SimpleURLLoader>>::iterator;

  APIRequestHelper(
      net::NetworkTrafficAnnotationTag annotation_tag,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~APIRequestHelper();

  using ResultCallback = base::OnceCallback<void(APIRequestResult)>;
  using ResponseConversionCallback =
      base::OnceCallback<absl::optional<std::string>(
          const std::string& raw_response)>;

  // Each response is expected in json format and will be validated through
  // JsonSanitizer. In cases where json contains values that are not supported
  // by the standard base/json parser it is necessary to convert such values
  // into string before validating the response. For these purposes
  // conversion_callback is added which receives raw response and can perform
  // necessary conversions.
  //
  // This method will be deprecated soon, please use the one underneath with
  // APIRequestOption parameter.
  // https://github.com/brave/brave-browser/issues/29611
  Ticket Request(
      const std::string& method,
      const GURL& url,
      const std::string& payload,
      const std::string& payload_content_type,
      bool auto_retry_on_network_change,
      ResultCallback callback,
      const base::flat_map<std::string, std::string>& headers = {},
      size_t max_body_size = -1u,
      ResponseConversionCallback conversion_callback = base::NullCallback());
  Ticket Request(
      const std::string& method,
      const GURL& url,
      const std::string& payload,
      const std::string& payload_content_type,
      ResultCallback callback,
      const APIRequestOptions& request_options,
      const base::flat_map<std::string, std::string>& headers = {},
      ResponseConversionCallback conversion_callback = base::NullCallback());

  using DownloadCallback = base::OnceCallback<void(
      base::FilePath,
      const base::flat_map<std::string, std::string>& /*response_headers*/)>;
  Ticket Download(const GURL& url,
                  const std::string& payload,
                  const std::string& payload_content_type,
                  bool auto_retry_on_network_change,
                  const base::FilePath& path,
                  DownloadCallback callback,
                  const base::flat_map<std::string, std::string>& headers = {});

  void Cancel(const Ticket& ticket);

 private:
  APIRequestHelper(const APIRequestHelper&) = delete;
  APIRequestHelper& operator=(const APIRequestHelper&) = delete;

  std::unique_ptr<network::SimpleURLLoader> CreateLoader(
      const std::string& method,
      const GURL& url,
      const std::string& payload,
      const std::string& payload_content_type,
      bool auto_retry_on_network_change,
      bool enable_cache,
      bool allow_http_error_result,
      const base::flat_map<std::string, std::string>& headers);

  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;
  void OnResponse(SimpleURLLoaderList::iterator iter,
                  ResultCallback callback,
                  ResponseConversionCallback conversion_callback,
                  const std::unique_ptr<std::string> response_body);
  void OnDownload(SimpleURLLoaderList::iterator iter,
                  DownloadCallback callback,
                  base::FilePath path);

  net::NetworkTrafficAnnotationTag annotation_tag_;
  SimpleURLLoaderList url_loaders_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::WeakPtrFactory<APIRequestHelper> weak_ptr_factory_{this};
};

}  // namespace api_request_helper

#endif  // BRAVE_COMPONENTS_API_REQUEST_HELPER_API_REQUEST_HELPER_H_
