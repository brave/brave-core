/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/auth_logout.h"

#include <optional>

#include "base/no_destructor.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoints/endpoint_test.h"
#include "net/base/net_errors.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoints {

bool operator==(const AuthLogout::Response::SuccessBody&,
                const AuthLogout::Response::SuccessBody&) {
  return true;
}

bool operator==(const AuthLogout::Response::ErrorBody&,
                const AuthLogout::Response::ErrorBody&) {
  return true;
}

namespace {

using AuthLogoutTestCase = EndpointTestCase<AuthLogout>;

const AuthLogoutTestCase* Success() {
  static const base::NoDestructor<AuthLogoutTestCase> kSuccess(
      {.test_name = "success",
       .http_status_code = net::HTTP_NO_CONTENT,
       .raw_response_body = "",
       .expected_response = {.net_error = net::OK,
                             .status_code = net::HTTP_NO_CONTENT,
                             .body = std::nullopt}});
  return kSuccess.get();
}

// clang-format off
// application/json errors:
// - HTTP 401:
//   - { "code": null, "error": "Unauthorized", "status": 401 }
// - HTTP 5XX:
//   - { "code": null, "error": "Internal Server Error", "status": <5xx> }
// clang-format on
const AuthLogoutTestCase* ApplicationJsonError() {
  static const base::NoDestructor<AuthLogoutTestCase> kApplicationJsonError(
      {.test_name = "application_json_error",
       .http_status_code = net::HTTP_UNAUTHORIZED,
       .raw_response_body =
           R"({ "code": null,
                "error": "Unauthorized",
                "status": 401 })",
       .expected_response = {
           .net_error = net::OK,
           .status_code = net::HTTP_UNAUTHORIZED,
           .body = base::unexpected(AuthLogout::Response::ErrorBody())}});
  return kApplicationJsonError.get();
}

// non-application/json errors:
// - HTTP 5XX:
//   - plain text errors returned by AWS/load balancer
const AuthLogoutTestCase* NonApplicationJsonError() {
  static const base::NoDestructor<AuthLogoutTestCase> kNonApplicationJsonError(
      {.test_name = "non_application_json_error",
       .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
       .raw_response_body = "non-application/json error",
       .expected_response = {.net_error = net::OK,
                             .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                             .body = std::nullopt}});
  return kNonApplicationJsonError.get();
}

using AuthLogoutTest = EndpointTest<AuthLogout>;

}  // namespace

TEST_P(AuthLogoutTest, HandlesReplies) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(AuthLogoutTestCases,
                         AuthLogoutTest,
                         testing::Values(Success(),
                                         ApplicationJsonError(),
                                         NonApplicationJsonError()),
                         AuthLogoutTest::kNameGenerator);

}  // namespace brave_account::endpoints
