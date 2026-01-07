/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/auth_validate.h"

#include "base/no_destructor.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoints/endpoint_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoints {

bool operator==(const AuthValidate::Response::SuccessBody& lhs,
                const AuthValidate::Response::SuccessBody& rhs) {
  return lhs.email == rhs.email;
}

namespace {

using AuthValidateTestCase = EndpointTestCase<AuthValidate>;

const AuthValidateTestCase* Success() {
  static const base::NoDestructor<AuthValidateTestCase> kSuccess(
      {.test_name = "success",
       .http_status_code = net::HTTP_OK,
       .raw_response_body = R"({ "email": "email" })",
       .expected_response = {
           .net_error = net::OK, .status_code = net::HTTP_OK, .body = [] {
             AuthValidate::Response::SuccessBody body;
             body.email = "email";
             return body;
           }()}});

  return kSuccess.get();
}

// clang-format off
// application/json errors:
// - HTTP 401:
//   - { "code": null, "error": "Unauthorized", "status": 401 }
// - HTTP 403:
//   - { "code": 14007, "error": "invalid token audience", "status": 403 }
// - HTTP 5XX:
//   - { "code": null, "error": "Internal Server Error", "status": <5xx> }
// clang-format on
const AuthValidateTestCase* ApplicationJsonErrorCodeIsNull() {
  static const base::NoDestructor<AuthValidateTestCase>
      kApplicationJsonErrorCodeIsNull(
          {.test_name = "application_json_error_code_is_null",
           .http_status_code = net::HTTP_UNAUTHORIZED,
           .raw_response_body =
               R"({ "code": null,
                    "error": "Unauthorized",
                    "status": 401 })",
           .expected_response = {.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   AuthValidate::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}});
  return kApplicationJsonErrorCodeIsNull.get();
}

const AuthValidateTestCase* ApplicationJsonErrorCodeIsNotNull() {
  static const base::NoDestructor<AuthValidateTestCase>
      kApplicationJsonErrorCodeIsNotNull(
          {.test_name = "application_json_error_code_is_not_null",
           .http_status_code = net::HTTP_FORBIDDEN,
           .raw_response_body =
               R"({ "code": 14007,
                    "error": "invalid token audience",
                    "status": 403 })",
           .expected_response = {.net_error = net::OK,
                                 .status_code = net::HTTP_FORBIDDEN,
                                 .body = base::unexpected([] {
                                   AuthValidate::Response::ErrorBody body;
                                   body.code = base::Value(14007);
                                   return body;
                                 }())}});
  return kApplicationJsonErrorCodeIsNotNull.get();
}

// non-application/json errors:
// - HTTP 5XX:
//   - plain text errors returned by AWS/load balancer
const AuthValidateTestCase* NonApplicationJsonError() {
  static const base::NoDestructor<AuthValidateTestCase>
      kNonApplicationJsonError(
          {.test_name = "non_application_json_error",
           .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
           .raw_response_body = "non-application/json error",
           .expected_response = {.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}});
  return kNonApplicationJsonError.get();
}

using AuthValidateTest = EndpointTest<AuthValidate>;

}  // namespace

TEST_P(AuthValidateTest, HandlesReplies) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(AuthValidateTestCases,
                         AuthValidateTest,
                         testing::Values(Success(),
                                         ApplicationJsonErrorCodeIsNull(),
                                         ApplicationJsonErrorCodeIsNotNull(),
                                         NonApplicationJsonError()),
                         AuthValidateTest::kNameGenerator);

}  // namespace brave_account::endpoints
