/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/service_token.h"

#include <optional>

#include "base/no_destructor.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoints/endpoint_test.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoints {

bool operator==(const ServiceToken::Response::SuccessBody& lhs,
                const ServiceToken::Response::SuccessBody& rhs) {
  return lhs.auth_token == rhs.auth_token;
}

namespace {

using ServiceTokenTestCase = EndpointTestCase<ServiceToken>;

const ServiceTokenTestCase* Success() {
  static const base::NoDestructor<ServiceTokenTestCase> kSuccess(
      {.test_name = "success",
       .http_status_code = net::HTTP_OK,
       .raw_response_body = R"({ "authToken": "34c375d933e3c" })",
       .expected_response = {
           .net_error = net::OK, .status_code = net::HTTP_OK, .body = [] {
             ServiceToken::Response::SuccessBody body;
             body.auth_token = "34c375d933e3c";
             return body;
           }()}});
  return kSuccess.get();
}

// clang-format off
// application/json errors:
// - HTTP 400:
//   - { "code": null, "error": "Bad Request", "status": 400 }
//   - { "code": 13006, "error": "email domain is not supported", "status": 400 }
// - HTTP 401:
//   - { "code": null, "error": "Unauthorized", "status": 401 }
// - HTTP 403:
//   - { "code": 14004, "error": "incorrect credentials", "status": 403 }
//   - { "code": 14007, "error": "invalid token audience", "status": 403 }
// - HTTP 5XX:
//   - { "code": null, "error": "Internal Server Error", "status": <5xx> }
// clang-format on
const ServiceTokenTestCase* ApplicationJsonErrorCodeIsNull() {
  static const base::NoDestructor<ServiceTokenTestCase>
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
                                   ServiceToken::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}});
  return kApplicationJsonErrorCodeIsNull.get();
}

const ServiceTokenTestCase* ApplicationJsonErrorCodeIsNotNull() {
  static const base::NoDestructor<ServiceTokenTestCase>
      kApplicationJsonErrorCodeIsNotNull(
          {.test_name = "application_json_error_code_is_not_null",
           .http_status_code = net::HTTP_BAD_REQUEST,
           .raw_response_body =
               R"({ "code": 13006,
                    "error": "email domain is not supported",
                    "status": 400 })",
           .expected_response = {.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   ServiceToken::Response::ErrorBody body;
                                   body.code = base::Value(13006);
                                   return body;
                                 }())}});
  return kApplicationJsonErrorCodeIsNotNull.get();
}

// non-application/json errors:
// - HTTP 5XX:
//   - plain text errors returned by AWS/load balancer
const ServiceTokenTestCase* NonApplicationJsonError() {
  static const base::NoDestructor<ServiceTokenTestCase>
      kNonApplicationJsonError(
          {.test_name = "non_application_json_error",
           .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
           .raw_response_body = "non-application/json error",
           .expected_response = {.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}});
  return kNonApplicationJsonError.get();
}

using ServiceTokenTest = EndpointTest<ServiceToken>;

}  // namespace

TEST_P(ServiceTokenTest, HandlesReplies) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(ServiceTokenTestCases,
                         ServiceTokenTest,
                         testing::Values(Success(),
                                         ApplicationJsonErrorCodeIsNull(),
                                         ApplicationJsonErrorCodeIsNotNull(),
                                         NonApplicationJsonError()),
                         ServiceTokenTest::kNameGenerator);

}  // namespace brave_account::endpoints
