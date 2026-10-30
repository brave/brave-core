/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_REQUEST_BODY_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_REQUEST_BODY_H_

#include "brave/components/brave_account/endpoint_client/has_to_value.h"
#include "brave/components/brave_account/endpoint_client/is_protobuf_message.h"

namespace brave_account::endpoint_client::detail {

// A JSON request body is a type that HasToValue.
template <typename RequestBody>
concept IsJSONRequestBody = HasToValue<RequestBody>;

// A Protobuf request body is a type that IsProtobufMessage.
template <typename RequestBody>
concept IsProtobufRequestBody = IsProtobufMessage<RequestBody>;

// A request body is either a JSON request body or a Protobuf request body.
template <typename RequestBody>
concept IsRequestBody =
    IsJSONRequestBody<RequestBody> || IsProtobufRequestBody<RequestBody>;

}  // namespace brave_account::endpoint_client::detail

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_REQUEST_BODY_H_
