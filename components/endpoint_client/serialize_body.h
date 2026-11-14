/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_ENDPOINT_CLIENT_SERIALIZE_BODY_H_
#define BRAVE_COMPONENTS_ENDPOINT_CLIENT_SERIALIZE_BODY_H_

#include <string>

#include "base/values.h"
#include "brave/components/endpoint_client/has_to_value.h"

namespace google::protobuf {
class MessageLite;
}  // namespace google::protobuf

namespace endpoint_client::internal {

// Serializes a base::DictValue to a JSON string.
std::string SerializeBody(const base::DictValue& body);

// Serializes a google::protobuf::MessageLite to a binary string.
std::string SerializeBody(const google::protobuf::MessageLite& body);

// Converts a JSON body via ToValue() and delegates serialization to the
// base::DictValue overload. Also used for JSON response bodies in
// test_support.h.
std::string SerializeBody(const HasToValue auto& body) {
  return SerializeBody(body.ToValue());
}

}  // namespace endpoint_client::internal

#endif  // BRAVE_COMPONENTS_ENDPOINT_CLIENT_SERIALIZE_BODY_H_
