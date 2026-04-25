/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_REQUEST_BODY_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_REQUEST_BODY_H_

#include <concepts>
#include <type_traits>

#include "base/values.h"
#include "google/protobuf/message_lite.h"

namespace brave_account::endpoint_client::detail {

// A JSON request body provides a non-static ToValue()
// callable on a const object and returning base::DictValue.
template <typename RequestBody>
concept IsJSONRequestBody = requires(const RequestBody& request_body) {
  { request_body.ToValue() } -> std::same_as<base::DictValue>;
} && std::is_member_function_pointer_v<decltype(&RequestBody::ToValue)>;

// A Protobuf request body is a type publicly derived from
// google::protobuf::MessageLite.
template <typename RequestBody>
concept IsProtobufRequestBody =
    std::derived_from<RequestBody, google::protobuf::MessageLite>;

// A request body is either a JSON request body or a Protobuf request body.
template <typename RequestBody>
concept IsRequestBody =
    IsJSONRequestBody<RequestBody> || IsProtobufRequestBody<RequestBody>;

}  // namespace brave_account::endpoint_client::detail

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_REQUEST_BODY_H_
