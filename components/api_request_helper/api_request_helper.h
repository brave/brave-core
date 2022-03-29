/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_API_REQUEST_HELPER_API_REQUEST_HELPER_H_
#define BRAVE_COMPONENTS_API_REQUEST_HELPER_API_REQUEST_HELPER_H_

#include <list>
#include <memory>
#include <string>

#include "base/callback.h"
#include "base/callback_helpers.h"
#include "base/containers/flat_map.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "url/gurl.h"

namespace network {
class SharedURLLoaderFactory;
class SimpleURLLoader;
}  // namespace network

namespace api_request_helper {

// Anyone is welcome to use APIRequestHelper to reduce boilerplate
class APIRequestHelper {
 public:
  APIRequestHelper(
      net::NetworkTrafficAnnotationTag annotation_tag,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory);
  ~APIRequestHelper();

  using ResultCallback =
      base::OnceCallback<void(const int,
                              const std::string&,
                              const base::flat_map<std::string, std::string>&)>;
  using ResponseConversionCallback =
      base::OnceCallback<absl::optional<std::string>(
          const std::string& raw_response)>;

  // Each response is expected in json format and will be validated through
  // JsonSanitizer. In cases where json contains values that are not supported
  // by the standard base/json parser it is necessary to convert such values
  // into string before validating the response. For these purposes
  // conversion_callback is added which receives raw response and can perform
  // necessary conversions.
  void Request(
      const std::string& method,
      const GURL& url,
      const std::string& payload,
      const std::string& payload_content_type,
      bool auto_retry_on_network_change,
      ResultCallback callback,
      const base::flat_map<std::string, std::string>& headers = {},
      size_t max_body_size = -1u,
      ResponseConversionCallback conversion_callback = base::NullCallback());

 private:
  APIRequestHelper(const APIRequestHelper&) = delete;
  APIRequestHelper& operator=(const APIRequestHelper&) = delete;
  using SimpleURLLoaderList =
      std::list<std::unique_ptr<network::SimpleURLLoader>>;
  void OnResponse(SimpleURLLoaderList::iterator iter,
                  ResultCallback callback,
                  ResponseConversionCallback conversion_callback,
                  const std::unique_ptr<std::string> response_body);

  net::NetworkTrafficAnnotationTag annotation_tag_;
  SimpleURLLoaderList url_loaders_;
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  base::WeakPtrFactory<APIRequestHelper> weak_ptr_factory_{this};
};

}  // namespace api_request_helper

#endif  // BRAVE_COMPONENTS_API_REQUEST_HELPER_API_REQUEST_HELPER_H_
