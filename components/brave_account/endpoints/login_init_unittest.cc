/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/login_init.h"

#include <optional>
#include <string>

#include "base/no_destructor.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoints/endpoint_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoints {

bool operator==(const LoginInit::Response::SuccessBody& lhs,
                const LoginInit::Response::SuccessBody& rhs) {
  return lhs.login_token == rhs.login_token &&
         lhs.serialized_ke2 == rhs.serialized_ke2;
}

namespace {

using LoginInitTestCase = EndpointTestCase<LoginInit>;

const LoginInitTestCase* Success() {
  static const base::NoDestructor<LoginInitTestCase> kSuccess(
      {.test_name = "success",
       .http_status_code = net::HTTP_OK,
       .raw_response_body = R"({ "loginToken": "eyJhbGciOiJFUz",
                                 "serializedKE2": "34c375d933e3c" })",
       .expected_response = {
           .net_error = net::OK, .status_code = net::HTTP_OK, .body = [] {
             LoginInit::Response::SuccessBody body;
             body.login_token = "eyJhbGciOiJFUz";
             body.serialized_ke2 = "34c375d933e3c";
             return body;
           }()}});

  return kSuccess.get();
}

// clang-format off
// application/json errors:
// - HTTP 400:
//   - { "code": null, "error": "Bad Request", "status": 400 }
// - HTTP 401:
//   - { "code": 11003, "error": "email not verified", "status": 401 }
//   - { "code": 14004, "error": "incorrect credentials", "status": 401 }
//   - { "code": 14005, "error": "incorrect email", "status": 401 }
//   - { "code": 14006, "error": "incorrect password", "status": 401 }
// - HTTP 5XX:
//   - { "code": 0, "error": "Internal Server Error", "status": <5xx> }
// clang-format on
const LoginInitTestCase* ApplicationJsonErrorCodeIsNull() {
  static const base::NoDestructor<LoginInitTestCase>
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
                                   LoginInit::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}});
  return kApplicationJsonErrorCodeIsNull.get();
}

const LoginInitTestCase* ApplicationJsonErrorCodeIsNotNull() {
  static const base::NoDestructor<LoginInitTestCase>
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
                                   LoginInit::Response::ErrorBody body;
                                   body.code = base::Value(14004);
                                   return body;
                                 }())}});
  return kApplicationJsonErrorCodeIsNotNull.get();
}

// non-application/json errors:
// - HTTP 5XX:
//   - plain text errors returned by AWS/load balancer
const LoginInitTestCase* NonApplicationJsonError() {
  static const base::NoDestructor<LoginInitTestCase> kNonApplicationJsonError(
      {.test_name = "non_application_json_error",
       .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
       .raw_response_body = "non-application/json error",
       .expected_response = {.net_error = net::OK,
                             .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                             .body = std::nullopt}});
  return kNonApplicationJsonError.get();
}

using LoginInitTest = EndpointTest<LoginInit>;

}  // namespace

TEST_P(LoginInitTest, HandlesReplies) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(LoginInitTestCases,
                         LoginInitTest,
                         testing::Values(Success(),
                                         ApplicationJsonErrorCodeIsNull(),
                                         ApplicationJsonErrorCodeIsNotNull(),
                                         NonApplicationJsonError()),
                         LoginInitTest::kNameGenerator);

}  // namespace brave_account::endpoints
