/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/verify_complete.h"

#include <optional>
#include <tuple>

#include "base/no_destructor.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoints/endpoint_test.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoints {

bool operator==(const VerifyComplete::Response::SuccessBody& lhs,
                const VerifyComplete::Response::SuccessBody& rhs) {
  return std::tie(lhs.auth_token, lhs.email) ==
         std::tie(rhs.auth_token, rhs.email);
}

namespace {

using VerifyCompleteTestCase = EndpointTestCase<VerifyComplete>;

const VerifyCompleteTestCase* Success() {
  static const base::NoDestructor<VerifyCompleteTestCase> kSuccess(
      {.test_name = "success",
       .http_status_code = net::HTTP_OK,
       .raw_response_body = R"({ "authToken": "34c375d933e3c",
                                 "email": "email",
                                 "service": "accounts" })",
       .expected_response = {
           .net_error = net::OK, .status_code = net::HTTP_OK, .body = [] {
             VerifyComplete::Response::SuccessBody body;
             body.auth_token = "34c375d933e3c";
             body.email = "email";
             return body;
           }()}});
  return kSuccess.get();
}

// clang-format off
// application/json errors:
// - HTTP 400:
//   - { "code": 13009, "error": "email already verified", "status": 400 }
//   - { "code": 13010, "error": "maximum code verification attempts exceeded", "status": 400 }
//   - { "code": 13011, "error": "invalid verification code", "status": 400 }
// - HTTP 401:
//   - { "code": null, "error": "Unauthorized", "status": 401 }
// - HTTP 404:
//   - { "code": 13002, "error": "verification not found or invalid id/code", "status": 404 }
// - HTTP 5XX:
//   - { "code": null, "error": "Internal Server Error", "status": <5xx> }
// clang-format on
const VerifyCompleteTestCase* ApplicationJsonErrorCodeIsNull() {
  static const base::NoDestructor<VerifyCompleteTestCase>
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
                                   VerifyComplete::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}});
  return kApplicationJsonErrorCodeIsNull.get();
}

const VerifyCompleteTestCase* ApplicationJsonErrorCodeIsNotNull() {
  static const base::NoDestructor<VerifyCompleteTestCase>
      kApplicationJsonErrorCodeIsNotNull(
          {.test_name = "application_json_error_code_is_not_null",
           .http_status_code = net::HTTP_NOT_FOUND,
           .raw_response_body =
               R"({ "code": 13002,
                    "error": "verification not found or invalid id/code",
                    "status": 404 })",
           .expected_response = {.net_error = net::OK,
                                 .status_code = net::HTTP_NOT_FOUND,
                                 .body = base::unexpected([] {
                                   VerifyComplete::Response::ErrorBody body;
                                   body.code = base::Value(13002);
                                   return body;
                                 }())}});
  return kApplicationJsonErrorCodeIsNotNull.get();
}

// non-application/json errors:
// - HTTP 5XX:
//   - plain text errors returned by AWS/load balancer
const VerifyCompleteTestCase* NonApplicationJsonError() {
  static const base::NoDestructor<VerifyCompleteTestCase>
      kNonApplicationJsonError(
          {.test_name = "non_application_json_error",
           .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
           .raw_response_body = "non-application/json error",
           .expected_response = {.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}});
  return kNonApplicationJsonError.get();
}

using VerifyCompleteTest = EndpointTest<VerifyComplete>;

}  // namespace

TEST_P(VerifyCompleteTest, HandlesReplies) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(VerifyCompleteTestCases,
                         VerifyCompleteTest,
                         testing::Values(Success(),
                                         ApplicationJsonErrorCodeIsNull(),
                                         ApplicationJsonErrorCodeIsNotNull(),
                                         NonApplicationJsonError()),
                         VerifyCompleteTest::kNameGenerator);

}  // namespace brave_account::endpoints
