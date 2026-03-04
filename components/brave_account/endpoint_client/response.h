/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_RESPONSE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_RESPONSE_H_

#include <concepts>
#include <optional>
#include <string>
#include <utility>

#include "base/json/json_reader.h"
#include "base/memory/scoped_refptr.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/is_response_body.h"
#include "brave/components/brave_account/endpoint_client/json_empty_body.h"
#include "brave/components/brave_account/endpoint_client/protobuf_empty_body.pb.h"
#include "net/base/net_errors.h"
#include "net/http/http_response_headers.h"
#include "services/network/public/cpp/header_util.h"

namespace brave_account::endpoint_client {

// A response contains the network result, HTTP status code, and the
// deserialized success or error body.
template <detail::IsResponseBody SuccessResponseBody,
          detail::IsResponseBody ErrorResponseBody>
struct Response {
  using SuccessBody = SuccessResponseBody;
  using ErrorBody = ErrorResponseBody;

  // Builds a Response from the network result, HTTP response headers,
  // and optional response body. Body parsing is only attempted for
  // net::OK responses with headers present.
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
    if (network::IsSuccessfulStatus(*response.status_code)) {  // 2xx
      response.body = Deserialize<SuccessBody>(std::move(response_body));
    } else {  // non-2xx
      response.body = Deserialize<ErrorBody>(std::move(response_body))
                          .transform([](auto error_body) {
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
  //
  // Behavior:
  //
  // 1. No body expected:
  //    (std::same_as<ResponseBody, JSONEmptyBody>):
  //    Whatever the server returns is ignored. We replace it with "{}" so the
  //    returned value is always a valid instance of the empty type, never
  //    std::nullopt. This also means parsing failures are ignored: if you don't
  //    expect data from the server, you shouldn't care whether what it sent was
  //    parseable or not.
  //
  // 2. Body expected:
  //    (!std::same_as<ResponseBody, JSONEmptyBody>):
  //    The returned value is std::nullopt if:
  //      - response_body is std::nullopt,
  //      - JSONReader::Read() fails,
  //      - ResponseBody::FromValue() fails.
  //
  //    The returned value is non-std::nullopt only when response_body contains
  //    valid JSON matching the expected ResponseBody structure.
  template <detail::IsJSONResponseBody ResponseBody>
  static auto Deserialize(std::optional<std::string> response_body) {
    if (std::same_as<ResponseBody, JSONEmptyBody>) {
      response_body = "{}";
    }

    const auto value =
        base::JSONReader::Read(response_body.value_or(""), base::JSON_PARSE_RFC)
            .value_or(base::Value());

    return ResponseBody::FromValue(value);
  }

  // Deserializes a Protobuf response body into ResponseBody.
  //
  // Behavior:
  //
  // 1. No body expected
  //    (std::same_as<ResponseBody, ProtobufEmptyBody>):
  //    Whatever the server returns is ignored. We skip parsing and treat the
  //    body as successfully deserialized, so the returned value is always a
  //    valid instance of the empty type, never std::nullopt. This also means
  //    parsing failures are ignored: if you don't expect
  //    data from the server, you shouldn't care whether what it sent was
  //    parseable or not.
  //
  // 2. Body expected
  //    (!std::same_as<ResponseBody, ProtobufEmptyBody>):
  //    The returned value is std::nullopt if:
  //      - response_body is std::nullopt,
  //      - response_body is empty,
  //      - parsing via ParseFromString() fails (invalid wire format).
  //
  //    The returned value is non-std::nullopt only when response_body is
  //    non-empty and can be successfully parsed into ResponseBody.
  //
  //    Note: successful parsing guarantees only wire-format compatibility with
  //    ResponseBody. Protobuf permits unknown fields, so semantic correctness
  //    must be validated by the caller.
  template <detail::IsProtobufResponseBody ResponseBody>
  static auto Deserialize(std::optional<std::string> response_body) {
    ResponseBody body;
    return std::same_as<ResponseBody, ProtobufEmptyBody> ||
                   (response_body && !response_body->empty() &&
                    body.ParseFromString(*response_body))
               ? std::optional(std::move(body))
               : std::nullopt;
  }
};

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_RESPONSE_H_
