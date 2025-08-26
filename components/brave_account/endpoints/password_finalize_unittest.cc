/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/password_finalize.h"

#include <optional>
#include <string>

#include "base/types/expected.h"
#include "brave/components/brave_account/endpoints/endpoint_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoints {

bool operator==(const PasswordFinalize::Response& lhs,
                const PasswordFinalize::Response& rhs) {
  return true;
}

using PasswordFinalizeTestCase = EndpointTestCase<PasswordFinalize>;

// TODO(https://github.com/brave/brave-browser/issues/48713): This is a case of
// `-Wexit-time-destructors` violation and `[[clang::no_destroy]]` has been
// added in the meantime to fix the build error. Remove this attribute and
// provide a proper fix.
[[clang::no_destroy]] const PasswordFinalizeTestCase kSuccess{
    .test_name = "success",
    .http_status_code = net::HTTP_OK,
    .raw_reply = R"({ "authToken": null,
                                                          "requiresEmailVerification": true,
                                                          "requiresTwoFA": false,
                                                          "sessionsInvalidated": false })",
    .reply = PasswordFinalize::Response()};

// clang-format off
// application/json errors:
// - HTTP 400:
//   - { "code": 14002, "error": "interim password state has expired", "status": 400 }
//   - { "code": 0, "error": "Bad Request", "status": 400 }
// - HTTP 401:
//   - { "code": 0, "error": "Unauthorized", "status": 401 }
// - HTTP 403:
//   - { "code": 0, "error": "Forbidden", "status": 403 }
// - HTTP 404:
//   - { "code": 14001, "error": "interim password state not found", "status": 404 }
// - HTTP 5XX:
//   - { "code": 0, "error": "Internal Server Error", "status": <5xx> }
// clang-format on
// TODO(https://github.com/brave/brave-browser/issues/48713): This is a case of
// `-Wexit-time-destructors` violation and `[[clang::no_destroy]]` has been
// added in the meantime to fix the build error. Remove this attribute and
// provide a proper fix.
[[clang::no_destroy]] const PasswordFinalizeTestCase kApplicationJsonError{
    .test_name = "application_json_error",
    .http_status_code = net::HTTP_BAD_REQUEST,
    .raw_reply =
        R"({ "code": 14002,
             "error": "interim password state has expired",
             "status": 400 })",
    .reply = base::unexpected([] {
      PasswordFinalize::Error error;
      error.code = 14002;
      error.error = "interim password state has expired";
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
[[clang::no_destroy]] const PasswordFinalizeTestCase kNonApplicationJsonError{
    .test_name = "non_application_json_error",
    .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
    .raw_reply = "non-application/json error",
    .reply = base::unexpected(std::nullopt)};

using PasswordFinalizeTest = EndpointTest<PasswordFinalize>;

TEST_P(PasswordFinalizeTest, HandlesReplies) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(PasswordFinalizeTestCases,
                         PasswordFinalizeTest,
                         testing::Values(&kSuccess,
                                         &kApplicationJsonError,
                                         &kNonApplicationJsonError),
                         PasswordFinalizeTest::NameGenerator);

}  // namespace brave_account::endpoints
