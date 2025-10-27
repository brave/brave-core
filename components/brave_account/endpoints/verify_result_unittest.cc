/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/verify_result.h"

#include <string>

#include "base/no_destructor.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoints/endpoint_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoints {

bool operator==(const VerifyResult::Response& lhs,
                const VerifyResult::Response& rhs) {
  return lhs.auth_token == rhs.auth_token;
}

using VerifyResultTestCase = EndpointTestCase<VerifyResult>;

namespace {

const VerifyResultTestCase* SuccessAuthTokenIsNull() {
  static const base::NoDestructor<VerifyResultTestCase> kSuccessAuthTokenIsNull(
      {.test_name = "success_auth_token_is_null",
       .http_status_code = net::HTTP_OK,
       .raw_reply = R"({ "authToken": null,
                         "email": "email",
                         "service": "accounts",
                         "verified": false })",
       .reply = [] {
         VerifyResult::Response response;
         response.auth_token = base::Value();
         return response;
       }()});
  return kSuccessAuthTokenIsNull.get();
}

const VerifyResultTestCase* SuccessAuthTokenIsNotNull() {
  static const base::NoDestructor<VerifyResultTestCase>
      kSuccessAuthTokenIsNotNull(
          {.test_name = "success_auth_token_is_not_null",
           .http_status_code = net::HTTP_OK,
           .raw_reply = R"({ "authToken": "34c375d933e3c",
                             "email": "email",
                             "service": "accounts",
                             "verified": true })",
           .reply = [] {
             VerifyResult::Response response;
             response.auth_token = base::Value("34c375d933e3c");
             return response;
           }()});
  return kSuccessAuthTokenIsNotNull.get();
}

// clang-format off
// application/json errors:
// - HTTP 400:
//   - { "code": null, "error": "Bad Request", "status": 400 }
// - HTTP 401:
//   - { "code": 0, "error": "Unauthorized", "status": 401 }
// - HTTP 404:
//   - { "code": 13002, "error": "verification not found or invalid id/code", "status": 404 }
// - HTTP 5XX:
//   - { "code": 0, "error": "Internal Server Error", "status": <5xx> }
// clang-format on
const VerifyResultTestCase* ApplicationJsonErrorCodeIsNull() {
  static const base::NoDestructor<VerifyResultTestCase> kApplicationJsonError(
      {.test_name = "application_json_error_code_is_null",
       .http_status_code = net::HTTP_BAD_REQUEST,
       .raw_reply =
           R"({ "code": null,
                "error": "Bad Request",
                "status": 400 })",
       .reply = base::unexpected([] {
         VerifyResult::Error error;
         error.code = base::Value();
         return error;
       }())});
  return kApplicationJsonError.get();
}

const VerifyResultTestCase* ApplicationJsonErrorCodeIsNotNull() {
  static const base::NoDestructor<VerifyResultTestCase> kApplicationJsonError(
      {.test_name = "application_json_error_code_is_not_null",
       .http_status_code = net::HTTP_NOT_FOUND,
       .raw_reply =
           R"({ "code": 13002,
                "error": "verification not found or invalid id/code",
                "status": 404 })",
       .reply = base::unexpected([] {
         VerifyResult::Error error;
         error.code = base::Value(13002);
         return error;
       }())});
  return kApplicationJsonError.get();
}

// non-application/json errors:
// - HTTP 5XX:
//   - plain text errors returned by AWS/load balancer
const VerifyResultTestCase* NonApplicationJsonError() {
  static const base::NoDestructor<VerifyResultTestCase>
      kNonApplicationJsonError(
          {.test_name = "non_application_json_error",
           .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
           .raw_reply = "non-application/json error",
           .reply = base::unexpected(std::nullopt)});
  return kNonApplicationJsonError.get();
}

}  // namespace

using VerifyResultTest = EndpointTest<VerifyResult>;

TEST_P(VerifyResultTest, HandlesReplies) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(VerifyResultTestCases,
                         VerifyResultTest,
                         testing::Values(SuccessAuthTokenIsNull(),
                                         SuccessAuthTokenIsNotNull(),
                                         ApplicationJsonErrorCodeIsNull(),
                                         ApplicationJsonErrorCodeIsNotNull(),
                                         NonApplicationJsonError()),
                         VerifyResultTest::kNameGenerator);

}  // namespace brave_account::endpoints
