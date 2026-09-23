/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_PUBLIC_CPP_UTILS_PROTOBUF_UTILS_H_
#define BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_PUBLIC_CPP_UTILS_PROTOBUF_UTILS_H_

#include <optional>
#include <string>

namespace brave_wallet {

// Prefixes provided serialized protobuf with compression byte and 4 bytes of
// message size. See
// https://github.com/grpc/grpc/blob/master/doc/PROTOCOL-HTTP2.md
std::string GetPrefixedProtobuf(const std::string& serialized_proto);

// Resolves serialized protobuf from length-prefixed string
std::optional<std::string> ResolveSerializedMessage(
    const std::string& grpc_response_body);

}  // namespace brave_wallet

#endif  // BRAVE_COMPONENTS_SERVICES_BRAVE_WALLET_PUBLIC_CPP_UTILS_PROTOBUF_UTILS_H_
