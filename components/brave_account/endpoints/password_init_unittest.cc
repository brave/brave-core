/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/password_init.h"

#include <string>

#include "base/no_destructor.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoints/endpoint_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoints {

bool operator==(const PasswordInit::Response& lhs,
                const PasswordInit::Response& rhs) {
  return lhs.serialized_response == rhs.serialized_response &&
         lhs.verification_token == rhs.verification_token;
}

using PasswordInitTestCase = EndpointTestCase<PasswordInit>;

namespace {

const PasswordInitTestCase* Success() {
  static const base::NoDestructor<PasswordInitTestCase> kSuccess(
      {.test_name = "success",
       .http_status_code = net::HTTP_OK,
       .raw_reply = R"({ "serializedResponse": "34c375d933e3c",
                         "verificationToken": "eyJhbGciOiJFUz" })",
       .reply = [] {
         PasswordInit::Response response;
         response.serialized_response = "34c375d933e3c";
         response.verification_token = "eyJhbGciOiJFUz";
         return response;
       }()});
  return kSuccess.get();
}

// clang-format off
// application/json errors:
// - HTTP 400:
//   - { "code": 11005, "error": "newAccountEmail is required when no verification token is provided", "status": 400 }
//   - { "code": 13001, "error": "too many pending verification requests for email", "status": 400 }
//   - { "code": 13003, "error": "intent not allowed", "status": 400 }
//   - { "code": 13004, "error": "account already exists", "status": 400 }
//   - { "code": 13006, "error": "email domain is not supported", "status": 400 }
// - HTTP 401:
//   - { "code": 0, "error": "Unauthorized", "status": 401 }
// - HTTP 5XX:
//   - { "code": 0, "error": "Internal Server Error", "status": <5xx> }
// clang-format on
const PasswordInitTestCase* ApplicationJsonError() {
  static const base::NoDestructor<PasswordInitTestCase> kApplicationJsonError(
      {.test_name = "application_json_error",
       .http_status_code = net::HTTP_BAD_REQUEST,
       .raw_reply =
           R"({ "code": 13004,
                "error": "account already exists",
                "status": 400 })",
       .reply = base::unexpected([] {
         PasswordInit::Error error;
         error.code = base::Value(13004);
         return error;
       }())});
  return kApplicationJsonError.get();
}

// non-application/json errors:
// - HTTP 5XX:
//   - plain text errors returned by AWS/load balancer
const PasswordInitTestCase* NonApplicationJsonError() {
  static const base::NoDestructor<PasswordInitTestCase>
      kNonApplicationJsonError(
          {.test_name = "non_application_json_error",
           .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
           .raw_reply = "non-application/json error",
           .reply = base::unexpected(endpoint_client::ParseError(
               "expected ident at line 1 column 2"))});
  return kNonApplicationJsonError.get();
}

}  // namespace

using PasswordInitTest = EndpointTest<PasswordInit>;

TEST_P(PasswordInitTest, HandlesReplies) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(PasswordInitTestCases,
                         PasswordInitTest,
                         testing::Values(Success(),
                                         ApplicationJsonError(),
                                         NonApplicationJsonError()),
                         PasswordInitTest::kNameGenerator);

}  // namespace brave_account::endpoints
