/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_TEST_ENDPOINT_BODIES_EQUALITY_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_TEST_ENDPOINT_BODIES_EQUALITY_H_

#include "brave/components/brave_account/endpoint_client/json_test_endpoint_bodies.h"
#include "brave/components/brave_account/endpoint_client/protobuf_test_endpoint_bodies.pb.h"

// Equality for the generated test endpoint bodies, which the schema compiler
// and Protobuf do not provide. Defined here rather than per test file so that
// unit tests linking into the same binary share one definition.

namespace brave_account::endpoint_client {

inline bool operator==(const JSONRequestBody& lhs, const JSONRequestBody& rhs) {
  return lhs.request == rhs.request;
}

inline bool operator==(const JSONSuccessBody& lhs, const JSONSuccessBody& rhs) {
  return lhs.success == rhs.success;
}

inline bool operator==(const JSONErrorBody& lhs, const JSONErrorBody& rhs) {
  return lhs.error == rhs.error;
}

inline bool operator==(const ProtobufRequestBody& lhs,
                       const ProtobufRequestBody& rhs) {
  return lhs.request() == rhs.request();
}

inline bool operator==(const ProtobufSuccessBody& lhs,
                       const ProtobufSuccessBody& rhs) {
  return lhs.success() == rhs.success();
}

inline bool operator==(const ProtobufErrorBody& lhs,
                       const ProtobufErrorBody& rhs) {
  return lhs.error() == rhs.error();
}

}  // namespace brave_account::endpoint_client

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_ENDPOINT_CLIENT_TEST_ENDPOINT_BODIES_EQUALITY_H_
