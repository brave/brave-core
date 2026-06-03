/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/verify_init.h"

#include <optional>

#include "base/no_destructor.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoints/endpoint_test.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoints {

bool operator==(const VerifyInit::Response::SuccessBody& lhs,
                const VerifyInit::Response::SuccessBody& rhs) {
  return lhs.verification_token == rhs.verification_token;
}

namespace {

using VerifyInitTestCase = EndpointTestCase<VerifyInit>;

const VerifyInitTestCase* Success() {
  static const base::NoDestructor<VerifyInitTestCase> kSuccess(
      {.test_name = "success",
       .http_status_code = net::HTTP_OK,
       .raw_response_body = R"({ "verificationToken": "eyJhbGciOiJFUz" })",
       .expected_response = {
           .net_error = net::OK, .status_code = net::HTTP_OK, .body = [] {
             VerifyInit::Response::SuccessBody body;
             body.verification_token = "eyJhbGciOiJFUz";
             return body;
           }()}});
  return kSuccess.get();
}

// clang-format off
// application/json errors:
// - HTTP 400:
//   - { "code": null, "error": "Bad Request", "status": 400 }
//   - { "code": 13001, "error": "too many pending verification requests for email", "status": 400 }
//   - { "code": 13003, "error": "intent not allowed", "status": 400 }
//   - { "code": 13004, "error": "account already exists", "status": 400 }
//   - { "code": 13005, "error": "account does not exist", "status": 400 }
//   - { "code": 13006, "error": "email domain is not supported", "status": 400 }
//   - { "code": 13007, "error": "failed to send email due to invalid format", "status": 400 }
// - HTTP 401:
//   - { "code": null, "error": "Unauthorized", "status": 401 }
// - HTTP 403:
//   - { "code": 14007, "error": "invalid token audience", "status": 403 }
// - HTTP 5XX:
//   - { "code": null, "error": "Internal Server Error", "status": <5xx> }
// clang-format on
const VerifyInitTestCase* ApplicationJsonErrorCodeIsNull() {
  static const base::NoDestructor<VerifyInitTestCase>
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
                                   VerifyInit::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}});
  return kApplicationJsonErrorCodeIsNull.get();
}

const VerifyInitTestCase* ApplicationJsonErrorCodeIsNotNull() {
  static const base::NoDestructor<VerifyInitTestCase>
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
                                   VerifyInit::Response::ErrorBody body;
                                   body.code = base::Value(14007);
                                   return body;
                                 }())}});
  return kApplicationJsonErrorCodeIsNotNull.get();
}

// non-application/json errors:
// - HTTP 5XX:
//   - plain text errors returned by AWS/load balancer
const VerifyInitTestCase* NonApplicationJsonError() {
  static const base::NoDestructor<VerifyInitTestCase> kNonApplicationJsonError(
      {.test_name = "non_application_json_error",
       .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
       .raw_response_body = "non-application/json error",
       .expected_response = {.net_error = net::OK,
                             .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                             .body = std::nullopt}});
  return kNonApplicationJsonError.get();
}

using VerifyInitTest = EndpointTest<VerifyInit>;

}  // namespace

TEST_P(VerifyInitTest, HandlesReplies) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(VerifyInitTestCases,
                         VerifyInitTest,
                         testing::Values(Success(),
                                         ApplicationJsonErrorCodeIsNull(),
                                         ApplicationJsonErrorCodeIsNotNull(),
                                         NonApplicationJsonError()),
                         VerifyInitTest::kNameGenerator);

}  // namespace brave_account::endpoints
