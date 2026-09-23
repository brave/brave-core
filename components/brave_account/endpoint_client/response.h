/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_RESPONSE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_RESPONSE_H_

#include <optional>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/memory/scoped_refptr.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/is_response_body.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/header_util.h"

namespace brave_account::endpoint_client {

// A response contains the network result, HTTP status code, and the
// deserialized response body, if any.
template <detail::IsResponseBody SuccessResponseBody,
          detail::IsResponseBody ErrorResponseBody>
struct Response {
  using SuccessBody = SuccessResponseBody;
  using ErrorBody = ErrorResponseBody;

  // Builds a Response from the network result, HTTP response headers,
  // and optional response body. Body deserialization is only attempted
  // when the network request succeeded, response headers are present,
  // and a response body was provided.
  static Response Deserialize(
      int net_error,
      const scoped_refptr<net::HttpResponseHeaders>& headers,
      std::optional<std::string> response_body) {
    Response response;
    response.net_error = net_error;

    if (response.net_error != net::OK || !headers) {
      return response;
    }

    response.status_code = headers->response_code();

    if (!response_body) {
      return response;
    }

    if (network::IsSuccessfulStatus(*response.status_code)) {  // 2xx
      response.body = Deserialize<SuccessBody>(*response_body);
    } else {  // non-2xx
      response.body = Deserialize<ErrorBody>(*response_body)
                          .transform([](ErrorBody error_body) {
                            return base::unexpected(std::move(error_body));
                          });
    }

    return response;
  }

  int net_error = net::ERR_IO_PENDING;
  std::optional<int> status_code;
  std::optional<base::expected<SuccessBody, ErrorBody>> body;

 private:
  // Deserializes a JSON response body into ResponseBody.
  template <detail::IsJSONResponseBody ResponseBody>
  static std::optional<ResponseBody> Deserialize(
      const std::string& response_body) {
    return base::JSONReader::Read(response_body, base::JSON_PARSE_RFC)
        .and_then(
            [](base::Value value) { return ResponseBody::FromValue(value); });
  }

  // Deserializes a Protobuf response body into ResponseBody.
  template <detail::IsProtobufResponseBody ResponseBody>
  static std::optional<ResponseBody> Deserialize(
      const std::string& response_body) {
    if (ResponseBody body; body.ParseFromString(response_body)) {
      return body;
    }

    return std::nullopt;
  }
};

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_RESPONSE_H_
