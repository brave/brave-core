/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/login_finalize.h"

#include <optional>
#include <string>

#include "base/no_destructor.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoints/endpoint_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoints {

bool operator==(const LoginFinalize::Response::SuccessBody& lhs,
                const LoginFinalize::Response::SuccessBody& rhs) {
  return lhs.auth_token == rhs.auth_token;
}

namespace {

using LoginFinalizeTestCase = EndpointTestCase<LoginFinalize>;

const LoginFinalizeTestCase* Success() {
  static const base::NoDestructor<LoginFinalizeTestCase> kSuccess(
      {.test_name = "success",
       .http_status_code = net::HTTP_OK,
       .raw_response_body = R"({ "authToken": "eyJhbGciOiJFUz" })",
       .expected_response = {
           .net_error = net::OK, .status_code = net::HTTP_OK, .body = [] {
             LoginFinalize::Response::SuccessBody body;
             body.auth_token = "eyJhbGciOiJFUz";
             return body;
           }()}});

  return kSuccess.get();
}

// clang-format off
// application/json errors:
// - HTTP 400:
//   - { "code": null, "error": "Bad Request", "status": 400 }
//   - { "code": 14009, "error": "interim password state mismatch", "status": 400 }
// - HTTP 401:
//   - { "code": 14001, "error": "interim password state not found", "status": 401 }
//   - { "code": 14002, "error": "interim password state has expired", "status": 401 }
//   - { "code": 14004, "error": "incorrect credentials", "status": 401 }
//   - { "code": 14005, "error": "incorrect email", "status": 401 }
//   - { "code": 14006, "error": "incorrect password", "status": 401 }
// - HTTP 5XX:
//   - { "code": 0, "error": "Internal Server Error", "status": <5xx> }
// clang-format on
const LoginFinalizeTestCase* ApplicationJsonErrorCodeIsNull() {
  static const base::NoDestructor<LoginFinalizeTestCase>
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
                                   LoginFinalize::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}});
  return kApplicationJsonErrorCodeIsNull.get();
}

const LoginFinalizeTestCase* ApplicationJsonErrorCodeIsNotNull() {
  static const base::NoDestructor<LoginFinalizeTestCase>
      kApplicationJsonErrorCodeIsNotNull(
          {.test_name = "application_json_error_code_is_not_null",
           .http_status_code = net::HTTP_UNAUTHORIZED,
           .raw_response_body =
               R"({ "code": 14004,
                    "error": "incorrect credentials",
                    "status": 401 })",
           .expected_response = {.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   LoginFinalize::Response::ErrorBody body;
                                   body.code = base::Value(14004);
                                   return body;
                                 }())}});
  return kApplicationJsonErrorCodeIsNotNull.get();
}

// non-application/json errors:
// - HTTP 5XX:
//   - plain text errors returned by AWS/load balancer
const LoginFinalizeTestCase* NonApplicationJsonError() {
  static const base::NoDestructor<LoginFinalizeTestCase>
      kNonApplicationJsonError(
          {.test_name = "non_application_json_error",
           .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
           .raw_response_body = "non-application/json error",
           .expected_response = {.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}});
  return kNonApplicationJsonError.get();
}

using LoginFinalizeTest = EndpointTest<LoginFinalize>;

}  // namespace

TEST_P(LoginFinalizeTest, HandlesReplies) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(LoginFinalizeTestCases,
                         LoginFinalizeTest,
                         testing::Values(Success(),
                                         ApplicationJsonErrorCodeIsNull(),
                                         ApplicationJsonErrorCodeIsNotNull(),
                                         NonApplicationJsonError()),
                         LoginFinalizeTest::kNameGenerator);

}  // namespace brave_account::endpoints
