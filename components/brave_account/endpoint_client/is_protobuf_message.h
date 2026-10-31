/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_PROTOBUF_MESSAGE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_PROTOBUF_MESSAGE_H_

#include <concepts>

#include "google/protobuf/message_lite.h"

namespace brave_account::endpoint_client::detail {

// A Protobuf message is a type publicly derived from
// google::protobuf::MessageLite.
template <typename T>
concept IsProtobufMessage = std::derived_from<T, google::protobuf::MessageLite>;

}  // namespace brave_account::endpoint_client::detail

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_IS_PROTOBUF_MESSAGE_H_
