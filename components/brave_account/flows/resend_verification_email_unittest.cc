/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/resend_verification_email.h"

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "base/test/task_environment.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_account/brave_account_service_test.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoints/verify_resend.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/mojom/resend_verification_email.mojom.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

using endpoints::VerifyResend;

struct ResendVerificationEmailTestCase {
  using Endpoint = VerifyResend;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::ResendVerificationEmailResultPtr,
                                      mojom::ResendVerificationEmailErrorPtr>;

  static void Run(const ResendVerificationEmailTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    AccountStatePrefs(pref_service)
        .AddVerification(EncryptedVerificationToken(),
                         mojom::VerificationIntent::NewLoggedOutIntent(
                             test_case.logged_out_verification_intent));

    authentication->ResendVerificationEmail(
        mojom::VerificationIntent::NewLoggedOutIntent(
            mojom::LoggedOutVerificationIntent::kRegistration),
        std::move(callback));
  }

  std::string test_name;
  mojom::LoggedOutVerificationIntent logged_out_verification_intent;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const ResendVerificationEmailTestCase*
ResendVerificationEmailCalledInWrongState() {
  static const base::NoDestructor<ResendVerificationEmailTestCase> kTestCase({
      .test_name = "resend_verification_email_called_in_wrong_state",
      .logged_out_verification_intent =
          mojom::LoggedOutVerificationIntent::kResetPassword,
      .fail_decryption = {},    // not used
      .endpoint_response = {},  // not used
      .mojo_expected =
          base::unexpected(mojom::ResendVerificationEmailError::NewClientError(
              mojom::ResendVerificationEmailClientError::New(
                  mojom::ResendVerificationEmailClientErrorCode::
                      kCalledInWrongState))),
  });
  return kTestCase.get();
}

const ResendVerificationEmailTestCase*
ResendVerificationEmailVerificationTokenDecryptionFailed() {
  static const base::NoDestructor<ResendVerificationEmailTestCase> kTestCase({
      .test_name =
          "resend_verification_email_verification_token_decryption_failed",
      .logged_out_verification_intent =
          mojom::LoggedOutVerificationIntent::kRegistration,
      .fail_decryption = true,
      .endpoint_response = {},  // not used
      .mojo_expected =
          base::unexpected(mojom::ResendVerificationEmailError::NewClientError(
              mojom::ResendVerificationEmailClientError::New(
                  mojom::ResendVerificationEmailClientErrorCode::
                      kVerificationTokenDecryptionFailed))),
  });
  return kTestCase.get();
}

const ResendVerificationEmailTestCase* ResendVerificationEmailSuccess() {
  static const base::NoDestructor<ResendVerificationEmailTestCase> kTestCase({
      .test_name = "resend_verification_email_success",
      .logged_out_verification_intent =
          mojom::LoggedOutVerificationIntent::kRegistration,
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_NO_CONTENT,
                             .body = std::nullopt}},
      .mojo_expected = mojom::ResendVerificationEmailResult::New(),
  });
  return kTestCase.get();
}

const ResendVerificationEmailTestCase*
ResendVerificationEmailBodyMissingOrFailedToParse() {
  static const base::NoDestructor<ResendVerificationEmailTestCase> kTestCase({
      .test_name = "resend_verification_email_body_missing_or_failed_to_parse",
      .logged_out_verification_intent =
          mojom::LoggedOutVerificationIntent::kRegistration,
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                             .body = std::nullopt}},
      .mojo_expected =
          base::unexpected(mojom::ResendVerificationEmailError::NewServerError(
              mojom::ResendVerificationEmailServerError::New(
                  net::HTTP_INTERNAL_SERVER_ERROR,
                  mojom::ResendVerificationEmailServerErrorCode::
                      kInvalidResponse))),
  });
  return kTestCase.get();
}

const ResendVerificationEmailTestCase*
ResendVerificationEmailUnexpectedSuccessBody() {
  static const base::NoDestructor<ResendVerificationEmailTestCase> kTestCase({
      .test_name = "resend_verification_email_unexpected_success_body",
      .logged_out_verification_intent =
          mojom::LoggedOutVerificationIntent::kRegistration,
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body = VerifyResend::Response::SuccessBody()}},
      .mojo_expected =
          base::unexpected(mojom::ResendVerificationEmailError::NewServerError(
              mojom::ResendVerificationEmailServerError::New(
                  net::HTTP_OK, mojom::ResendVerificationEmailServerErrorCode::
                                    kInvalidResponse))),
  });
  return kTestCase.get();
}

const ResendVerificationEmailTestCase*
ResendVerificationEmailErrorCodeIsNull() {
  static const base::NoDestructor<ResendVerificationEmailTestCase> kTestCase({
      .test_name = "resend_verification_email_error_code_is_null",
      .logged_out_verification_intent =
          mojom::LoggedOutVerificationIntent::kRegistration,
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_BAD_REQUEST,
                             .body = base::unexpected([] {
                               VerifyResend::Response::ErrorBody body;
                               body.code = base::Value();
                               return body;
                             }())}},
      .mojo_expected =
          base::unexpected(mojom::ResendVerificationEmailError::NewServerError(
              mojom::ResendVerificationEmailServerError::New(
                  net::HTTP_BAD_REQUEST,
                  mojom::ResendVerificationEmailServerErrorCode::kNull))),
  });
  return kTestCase.get();
}

const ResendVerificationEmailTestCase*
ResendVerificationEmailUnknownErrorCode() {
  static const base::NoDestructor<ResendVerificationEmailTestCase> kTestCase({
      .test_name = "resend_verification_email_unknown_error_code",
      .logged_out_verification_intent =
          mojom::LoggedOutVerificationIntent::kRegistration,
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_TOO_EARLY,
                             .body = base::unexpected([] {
                               VerifyResend::Response::ErrorBody body;
                               body.code = base::Value(42);
                               return body;
                             }())}},
      .mojo_expected =
          base::unexpected(mojom::ResendVerificationEmailError::NewServerError(
              mojom::ResendVerificationEmailServerError::New(
                  net::HTTP_TOO_EARLY,
                  mojom::ResendVerificationEmailServerErrorCode::kUnknown))),
  });
  return kTestCase.get();
}

const ResendVerificationEmailTestCase* ResendVerificationEmailKnownErrorCode() {
  static const base::NoDestructor<ResendVerificationEmailTestCase> kTestCase({
      .test_name = "resend_verification_email_known_error_code",
      .logged_out_verification_intent =
          mojom::LoggedOutVerificationIntent::kRegistration,
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_BAD_REQUEST,
                             .body = base::unexpected([] {
                               VerifyResend::Response::ErrorBody body;
                               body.code = base::Value(13008);
                               return body;
                             }())}},
      .mojo_expected =
          base::unexpected(mojom::ResendVerificationEmailError::NewServerError(
              mojom::ResendVerificationEmailServerError::New(
                  net::HTTP_BAD_REQUEST,
                  mojom::ResendVerificationEmailServerErrorCode::
                      kMaximumEmailSendAttemptsExceeded))),
  });
  return kTestCase.get();
}

using BraveAccountServiceResendVerificationEmailTest =
    BraveAccountServiceTest<ResendVerificationEmailTestCase>;

}  // namespace

TEST_P(BraveAccountServiceResendVerificationEmailTest,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceResendVerificationEmailTest,
    testing::Values(ResendVerificationEmailCalledInWrongState(),
                    ResendVerificationEmailVerificationTokenDecryptionFailed(),
                    ResendVerificationEmailSuccess(),
                    ResendVerificationEmailBodyMissingOrFailedToParse(),
                    ResendVerificationEmailUnexpectedSuccessBody(),
                    ResendVerificationEmailErrorCodeIsNull(),
                    ResendVerificationEmailUnknownErrorCode(),
                    ResendVerificationEmailKnownErrorCode()),
    BraveAccountServiceResendVerificationEmailTest::kNameGenerator);

}  // namespace brave_account
