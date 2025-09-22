/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/password_init.h"

#include <optional>
#include <string>

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

// TODO(https://github.com/brave/brave-browser/issues/48713): This is a case of
// `-Wexit-time-destructors` violation and `[[clang::no_destroy]]` has been
// added in the meantime to fix the build error. Remove this attribute and
// provide a proper fix.
[[clang::no_destroy]] const PasswordInitTestCase kSuccess{
    .test_name = "success",
    .http_status_code = net::HTTP_OK,
    .raw_reply = R"({ "serializedResponse": "34c375d933e3c",
                      "verificationToken": "eyJhbGciOiJFUz" })",
    .reply = [] {
      PasswordInit::Response response;
      response.serialized_response = "34c375d933e3c";
      response.verification_token = "eyJhbGciOiJFUz";
      return response;
    }()};

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
// TODO(https://github.com/brave/brave-browser/issues/48713): This is a case of
// `-Wexit-time-destructors` violation and `[[clang::no_destroy]]` has been
// added in the meantime to fix the build error. Remove this attribute and
// provide a proper fix.
[[clang::no_destroy]] const PasswordInitTestCase kApplicationJsonError{
    .test_name = "application_json_error",
    .http_status_code = net::HTTP_BAD_REQUEST,
    .raw_reply =
        R"({ "code": 13004,
             "error": "account already exists",
             "status": 400 })",
    .reply = base::unexpected([] {
      PasswordInit::Error error;
      error.code = 13004;
      error.error = "account already exists";
      error.status = 400;
      return error;
    }())};

// non-application/json errors:
// - HTTP 5XX:
//   - plain text errors returned by AWS/load balancer
// TODO(https://github.com/brave/brave-browser/issues/48713): This is a case of
// `-Wexit-time-destructors` violation and `[[clang::no_destroy]]` has been
// added in the meantime to fix the build error. Remove this attribute and
// provide a proper fix.
[[clang::no_destroy]] const PasswordInitTestCase kNonApplicationJsonError{
    .test_name = "non_application_json_error",
    .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
    .raw_reply = "non-application/json error",
    .reply = base::unexpected(std::nullopt)};

using PasswordInitTest = EndpointTest<PasswordInit>;

TEST_P(PasswordInitTest, HandlesReplies) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(PasswordInitTestCases,
                         PasswordInitTest,
                         testing::Values(&kSuccess,
                                         &kApplicationJsonError,
                                         &kNonApplicationJsonError),
                         PasswordInitTest::NameGenerator);

}  // namespace brave_account::endpoints
