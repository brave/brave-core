/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/endpoints/verify_resend.h"

#include "base/no_destructor.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/endpoints/endpoint_test.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account::endpoints {

bool operator==(const VerifyResend::Response::SuccessBody&,
                const VerifyResend::Response::SuccessBody&) {
  return true;
}

namespace {

using VerifyResendTestCase = EndpointTestCase<VerifyResend>;

const VerifyResendTestCase* Success() {
  static const base::NoDestructor<VerifyResendTestCase> kSuccess(
      {.test_name = "success",
       .http_status_code = net::HTTP_NO_CONTENT,
       .raw_response_body = "",
       .expected_response = {.net_error = net::OK,
                             .status_code = net::HTTP_NO_CONTENT,
                             .body = VerifyResend::Response::SuccessBody()}});

  return kSuccess.get();
}

// clang-format off
// application/json errors:
// - HTTP 400:
//   - { "code": null, "error": "Bad Request", "status": 400 }
//   - { "code": 13008, "error": "maximum email send attempts exceeded", "status": 400 }
//   - { "code": 13009, "error": "email already verified", "status": 400 }
// - HTTP 5XX:
//   - { "code": null, "error": "Internal Server Error", "status": <5xx> }
// clang-format on
const VerifyResendTestCase* ApplicationJsonErrorCodeIsNull() {
  static const base::NoDestructor<VerifyResendTestCase>
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
                                   VerifyResend::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}});
  return kApplicationJsonErrorCodeIsNull.get();
}

const VerifyResendTestCase* ApplicationJsonErrorCodeIsNotNull() {
  static const base::NoDestructor<VerifyResendTestCase>
      kApplicationJsonErrorCodeIsNotNull(
          {.test_name = "application_json_error_code_is_not_null",
           .http_status_code = net::HTTP_BAD_REQUEST,
           .raw_response_body =
               R"({ "code": 13008,
                    "error": "maximum email send attempts exceeded",
                    "status": 400 })",
           .expected_response = {.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   VerifyResend::Response::ErrorBody body;
                                   body.code = base::Value(13008);
                                   return body;
                                 }())}});
  return kApplicationJsonErrorCodeIsNotNull.get();
}

// non-application/json errors:
// - HTTP 5XX:
//   - plain text errors returned by AWS/load balancer
const VerifyResendTestCase* NonApplicationJsonError() {
  static const base::NoDestructor<VerifyResendTestCase>
      kNonApplicationJsonError(
          {.test_name = "non_application_json_error",
           .http_status_code = net::HTTP_INTERNAL_SERVER_ERROR,
           .raw_response_body = "non-application/json error",
           .expected_response = {.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}});
  return kNonApplicationJsonError.get();
}

using VerifyResendTest = EndpointTest<VerifyResend>;

}  // namespace

TEST_P(VerifyResendTest, HandlesReplies) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(VerifyResendTestCases,
                         VerifyResendTest,
                         testing::Values(Success(),
                                         ApplicationJsonErrorCodeIsNull(),
                                         ApplicationJsonErrorCodeIsNotNull(),
                                         NonApplicationJsonError()),
                         VerifyResendTest::kNameGenerator);

}  // namespace brave_account::endpoints
