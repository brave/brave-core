/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_DESERIALIZE_BODY_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_DESERIALIZE_BODY_H_

#include <optional>
#include <string_view>

#include "base/json/json_reader.h"
#include "base/values.h"
#include "brave/components/endpoint_client/is_response_body.h"

namespace endpoint_client::internal {

// Deserializes a JSON response body into ResponseBody.
template <IsJSONResponseBody ResponseBody>
std::optional<ResponseBody> DeserializeBody(std::string_view response_body) {
  return base::JSONReader::Read(response_body, base::JSON_PARSE_RFC)
      .and_then(
          [](base::Value value) { return ResponseBody::FromValue(value); });
}

// Deserializes a Protobuf response body into ResponseBody.
template <IsProtobufResponseBody ResponseBody>
std::optional<ResponseBody> DeserializeBody(std::string_view response_body) {
  if (ResponseBody body; body.ParseFromString(response_body)) {
    return body;
  }

  return std::nullopt;
}

}  // namespace endpoint_client::internal

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_DESERIALIZE_BODY_H_
