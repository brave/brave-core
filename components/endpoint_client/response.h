/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_RESPONSE_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_RESPONSE_H_

#include <optional>
#include <string_view>
#include <utility>

#include "base/json/json_reader.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/endpoint_client/is_response_body.h"
#include "net/base/net_errors.h"
#include "services/network/public/cpp/header_util.h"

namespace endpoint_client {

template <typename...>
struct Response {
  static_assert(false,
                "Response<> takes either no body types or a success and an "
                "error body type!");
};

// A body-less response contains the network result and HTTP status code.
template <>
struct Response<> {
  int net_error = net::ERR_IO_PENDING;
  std::optional<int> status_code;
};

// A response with bodies contains the network result, HTTP status code, and
// the deserialized response body, if any.
template <internal::IsResponseBody SuccessResponseBody,
          internal::IsResponseBody ErrorResponseBody>
struct Response<SuccessResponseBody, ErrorResponseBody> {
  using SuccessBody = SuccessResponseBody;
  using ErrorBody = ErrorResponseBody;

  // Deserializes the response body into the success body for a 2xx status
  // code, and into the error body otherwise. Returns std::nullopt if the body
  // fails to parse.
  static std::optional<base::expected<SuccessBody, ErrorBody>> Deserialize(
      int status_code,
      std::string_view response_body) {
    std::optional<base::expected<SuccessBody, ErrorBody>> body;
    if (network::IsSuccessfulStatus(status_code)) {  // 2xx
      body = DeserializeBody<SuccessBody>(response_body);
    } else {  // non-2xx
      body = DeserializeBody<ErrorBody>(response_body)
                 .transform([](ErrorBody error_body) {
                   return base::unexpected(std::move(error_body));
                 });
    }

    return body;
  }

  int net_error = net::ERR_IO_PENDING;
  std::optional<int> status_code;
  std::optional<base::expected<SuccessBody, ErrorBody>> body;

 private:
  // Deserializes a JSON response body into ResponseBody.
  template <internal::IsJSONResponseBody ResponseBody>
  static std::optional<ResponseBody> DeserializeBody(
      std::string_view response_body) {
    return base::JSONReader::Read(response_body, base::JSON_PARSE_RFC)
        .and_then(
            [](base::Value value) { return ResponseBody::FromValue(value); });
  }

  // Deserializes a Protobuf response body into ResponseBody.
  template <internal::IsProtobufResponseBody ResponseBody>
  static std::optional<ResponseBody> DeserializeBody(
      std::string_view response_body) {
    if (ResponseBody body; body.ParseFromString(response_body)) {
      return body;
    }

    return std::nullopt;
  }
};

}  // namespace endpoint_client

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_RESPONSE_H_
