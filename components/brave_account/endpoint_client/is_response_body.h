/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_RESPONSE_BODY_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_RESPONSE_BODY_H_

#include <concepts>
#include <optional>

#include "base/values.h"
#include "brave/components/brave_account/endpoint_client/is_protobuf_message.h"

namespace brave_account::endpoint_client::detail {

// A JSON response body provides a static FromValue() taking
// const base::Value& and returning std::optional<ResponseBody>.
template <typename ResponseBody>
concept IsJSONResponseBody = requires(const base::Value& value) {
  {
    ResponseBody::FromValue(value)
  } -> std::same_as<std::optional<ResponseBody>>;
};

// A Protobuf response body is a type that IsProtobufMessage.
template <typename ResponseBody>
concept IsProtobufResponseBody = IsProtobufMessage<ResponseBody>;

// A response body is either a JSON response body or a Protobuf response body.
template <typename ResponseBody>
concept IsResponseBody =
    IsJSONResponseBody<ResponseBody> || IsProtobufResponseBody<ResponseBody>;

}  // namespace brave_account::endpoint_client::detail

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_RESPONSE_BODY_H_
