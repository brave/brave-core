/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/password_finalize.h"

#include <optional>

#include "base/no_destructor.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoints/endpoint_test.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoints {

bool operator==(const PasswordFinalize::Response::SuccessBody&,
                const PasswordFinalize::Response::SuccessBody&) {
  return true;
}

namespace {

using PasswordFinalizeTestCase = EndpointTestCase<PasswordFinalize>;

const PasswordFinalizeTestCase* Success() {
  static const base::NoDestructor<PasswordFinalizeTestCase> kSuccess(
      {.test_name = "success",
       .http_status_code = net::HTTP_OK,
       .raw_response_body = R"({ "authToken": null,
                                 "requiresEmailVerification": true,
                                 "requiresTwoFA": false,
                                 "sessionsInvalidated": false })",
       .expected_response = {
           .net_error = net::OK,
           .status_code = net::HTTP_OK,
           .body = PasswordFinalize::Response::SuccessBody()}});
  return kSuccess.get();
}

// clang-format off
// application/json errors:
// - HTTP 400:
//   - { "code": null, "error": "Bad Request", "status": 400 }
//   - { "code": 14002, "error": "interim password state has expired", "status": 400 }
// - HTTP 401:
//   - { "code": null, "error": "Unauthorized", "status": 401 }
// - HTTP 403:
//   - { "code": null, "error": "Forbidden", "status": 403 }
// - HTTP 404:
//   - { "code": 14001, "error": "interim password state not found", "status": 404 }
// - HTTP 5XX:
//   - { "code": null, "error": "Internal Server Error", "status": <5xx> }
// clang-format on
const PasswordFinalizeTestCase* ApplicationJsonErrorCodeIsNull() {
  static const base::NoDestructor<PasswordFinalizeTestCase>
      kApplicationJsonErrorCodeIsNull(
          {.test_name = "application_json_error_code_is_null",
           .http_status_code = net::HTTP_BAD_REQUEST,
           .raw_response_body =
               R"({ "code": null,
                    "error": "Bad Request",
                    "status": 400 })",
           .expected_response = {.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}});
  return kApplicationJsonErrorCodeIsNull.get();
}

const PasswordFinalizeTestCase* ApplicationJsonErrorCodeIsNotNull() {
  static const base::NoDestructor<PasswordFinalizeTestCase>
      kApplicationJsonErrorCodeIsNotNull(
          {.test_name = "application_json_error_code_is_not_null",
           .http_status_code = net::HTTP_BAD_REQUEST,
           .raw_response_body =
               R"({ "code": 14002,
                    "error": "interim password state has expired",
                    "status": 400 })",
           .expected_response = {.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value(14002);
                                   return body;
                                 }())}});
  return kApplicationJsonErrorCodeIsNotNull.get();
}

// non-application/json errors:
// - HTTP 5XX:
//   - plain text errors returned by AWS/load balancer
const PasswordFinalizeTestCase* NonApplicationJsonError() {
  static const base::NoDestructor<PasswordFinalizeTestCase>
      kNonApplicationJsonError(
          {.test_name = "non_application_json_error",
           .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
           .raw_response_body = "non-application/json error",
           .expected_response = {.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}});
  return kNonApplicationJsonError.get();
}

using PasswordFinalizeTest = EndpointTest<PasswordFinalize>;

}  // namespace

TEST_P(PasswordFinalizeTest, HandlesReplies) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(PasswordFinalizeTestCases,
                         PasswordFinalizeTest,
                         testing::Values(Success(),
                                         ApplicationJsonErrorCodeIsNull(),
                                         ApplicationJsonErrorCodeIsNotNull(),
                                         NonApplicationJsonError()),
                         PasswordFinalizeTest::kNameGenerator);

}  // namespace brave_account::endpoints
