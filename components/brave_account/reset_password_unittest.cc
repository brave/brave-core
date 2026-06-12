/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/reset_password.h"

#include <optional>
#include <string>

#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "base/test/task_environment.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_account/brave_account_service_test.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoints/password_finalize.h"
#include "brave/components/brave_account/endpoints/password_init.h"
#include "brave/components/brave_account/endpoints/verify_complete.h"
#include "brave/components/brave_account/endpoints/verify_init.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

using endpoints::PasswordFinalize;
using endpoints::PasswordInit;
using endpoints::VerifyComplete;
using endpoints::VerifyInit;

// `ResetPasswordVerifyComplete`, `ResetPasswordPasswordInit`, and
// `ResetPasswordPasswordFinalize` all require a pending `kResetPassword`
// verification slot in prefs (the token set by `ResetPasswordVerifyInit`).
// Seed it before invoking the method under test.
namespace {

void SeedResetPasswordVerification(PrefService& pref_service) {
  AccountStatePrefs(pref_service)
      .SetLoggedOutWithVerification(
          EncryptedVerificationToken(),
          mojom::LoggedOutVerificationIntent::kResetPassword);
}

}  // namespace

struct ResetPasswordVerifyInitTestCase {
  using Endpoint = VerifyInit;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::ResetPasswordVerifyInitResultPtr,
                                      mojom::ResetPasswordErrorPtr>;

  static void Run(const ResetPasswordVerifyInitTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    authentication->ResetPasswordVerifyInit(
        mojom::Service::kAccounts, kEmailAddress,
        std::move(callback).Then(base::BindOnce(
            [](PrefService* pref_service, bool success) {
              AccountStatePrefs account_state_prefs(*pref_service);
              const auto state = account_state_prefs.GetAccountState();
              ASSERT_TRUE(state->is_logged_out());
              if (success) {
                ASSERT_TRUE(state->get_logged_out()->verification);
                EXPECT_EQ(state->get_logged_out()->verification->intent,
                          mojom::LoggedOutVerificationIntent::kResetPassword);
                EXPECT_EQ(account_state_prefs.GetVerificationToken(
                              mojom::VerificationIntent::NewLoggedOutIntent(
                                  mojom::LoggedOutVerificationIntent::
                                      kResetPassword)),
                          EncryptedVerificationToken());
              } else {
                EXPECT_FALSE(state->get_logged_out()->verification);
              }
            },
            base::Unretained(&pref_service),
            test_case.mojo_expected.has_value())));
  }

  std::string test_name;
  bool fail_encryption;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const ResetPasswordVerifyInitTestCase*
ResetPasswordVerifyInitBodyMissingOrFailedToParse() {
  static const base::NoDestructor<ResetPasswordVerifyInitTestCase>
      kResetPasswordVerifyInitBodyMissingOrFailedToParse({
          .test_name =
              "reset_password_verify_init_body_missing_or_failed_to_parse",
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_INTERNAL_SERVER_ERROR,
                      mojom::ResetPasswordServerErrorCode::kInvalidResponse))),
      });
  return kResetPasswordVerifyInitBodyMissingOrFailedToParse.get();
}

const ResetPasswordVerifyInitTestCase*
ResetPasswordVerifyInitErrorCodeIsNull() {
  static const base::NoDestructor<ResetPasswordVerifyInitTestCase>
      kResetPasswordVerifyInitErrorCodeIsNull({
          .test_name = "reset_password_verify_init_error_code_is_null",
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   VerifyInit::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::ResetPasswordServerErrorCode::kNull))),
      });
  return kResetPasswordVerifyInitErrorCodeIsNull.get();
}

const ResetPasswordVerifyInitTestCase*
ResetPasswordVerifyInitUnknownErrorCode() {
  static const base::NoDestructor<ResetPasswordVerifyInitTestCase>
      kResetPasswordVerifyInitUnknownErrorCode({
          .test_name = "reset_password_verify_init_unknown_error_code",
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_TOO_EARLY,
                                 .body = base::unexpected([] {
                                   VerifyInit::Response::ErrorBody body;
                                   body.code = base::Value(42);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_TOO_EARLY,
                      mojom::ResetPasswordServerErrorCode::kUnknown))),
      });
  return kResetPasswordVerifyInitUnknownErrorCode.get();
}

const ResetPasswordVerifyInitTestCase* ResetPasswordVerifyInitKnownErrorCode() {
  static const base::NoDestructor<ResetPasswordVerifyInitTestCase>
      kResetPasswordVerifyInitKnownErrorCode({
          .test_name = "reset_password_verify_init_known_error_code",
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   VerifyInit::Response::ErrorBody body;
                                   body.code = base::Value(13005);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::ResetPasswordServerErrorCode::
                          kAccountDoesNotExist))),
      });
  return kResetPasswordVerifyInitKnownErrorCode.get();
}

const ResetPasswordVerifyInitTestCase*
ResetPasswordVerifyInitVerificationTokenEmpty() {
  static const base::NoDestructor<ResetPasswordVerifyInitTestCase>
      kResetPasswordVerifyInitVerificationTokenEmpty({
          .test_name = "reset_password_verify_init_verification_token_empty",
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       VerifyInit::Response::SuccessBody body;
                                       body.verification_token = "";
                                       return body;
                                     }()}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_OK,
                      mojom::ResetPasswordServerErrorCode::kInvalidResponse))),
      });
  return kResetPasswordVerifyInitVerificationTokenEmpty.get();
}

const ResetPasswordVerifyInitTestCase*
ResetPasswordVerifyInitVerificationTokenEncryptionFailed() {
  static const base::NoDestructor<ResetPasswordVerifyInitTestCase>
      kResetPasswordVerifyInitVerificationTokenEncryptionFailed({
          .test_name =
              "reset_password_verify_init_verification_token_encryption_failed",
          .fail_encryption = true,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       VerifyInit::Response::SuccessBody body;
                                       body.verification_token =
                                           kVerificationToken;
                                       return body;
                                     }()}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewClientError(
                  mojom::ResetPasswordClientError::New(
                      mojom::ResetPasswordClientErrorCode::
                          kVerificationTokenEncryptionFailed))),
      });
  return kResetPasswordVerifyInitVerificationTokenEncryptionFailed.get();
}

const ResetPasswordVerifyInitTestCase* ResetPasswordVerifyInitSuccess() {
  static const base::NoDestructor<ResetPasswordVerifyInitTestCase>
      kResetPasswordVerifyInitSuccess({
          .test_name = "reset_password_verify_init_success",
          .fail_encryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       VerifyInit::Response::SuccessBody body;
                                       body.verification_token =
                                           kVerificationToken;
                                       return body;
                                     }()}},
          .mojo_expected = mojom::ResetPasswordVerifyInitResult::New(),
      });
  return kResetPasswordVerifyInitSuccess.get();
}

using BraveAccountServiceResetPasswordVerifyInitTest =
    BraveAccountServiceTest<ResetPasswordVerifyInitTestCase>;

}  // namespace

TEST_P(BraveAccountServiceResetPasswordVerifyInitTest,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceResetPasswordVerifyInitTest,
    testing::Values(ResetPasswordVerifyInitBodyMissingOrFailedToParse(),
                    ResetPasswordVerifyInitErrorCodeIsNull(),
                    ResetPasswordVerifyInitUnknownErrorCode(),
                    ResetPasswordVerifyInitKnownErrorCode(),
                    ResetPasswordVerifyInitVerificationTokenEmpty(),
                    ResetPasswordVerifyInitVerificationTokenEncryptionFailed(),
                    ResetPasswordVerifyInitSuccess()),
    BraveAccountServiceResetPasswordVerifyInitTest::kNameGenerator);

struct ResetPasswordVerifyCompleteTestCase {
  using Endpoint = VerifyComplete;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected =
      base::expected<mojom::ResetPasswordVerifyCompleteResultPtr,
                     mojom::ResetPasswordErrorPtr>;

  static void Run(const ResetPasswordVerifyCompleteTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    if (test_case.seed_verification) {
      SeedResetPasswordVerification(pref_service);
    }

    authentication->ResetPasswordVerifyComplete(
        "23TZMP",
        std::move(callback).Then(base::BindOnce(
            [](PrefService* pref_service, bool success) {
              if (!success) {
                return;
              }

              AccountStatePrefs account_state_prefs(*pref_service);
              const auto state = account_state_prefs.GetAccountState();
              ASSERT_TRUE(state->is_logged_out());
              ASSERT_TRUE(state->get_logged_out()->verification);
              EXPECT_EQ(state->get_logged_out()->verification->intent,
                        mojom::LoggedOutVerificationIntent::kResetPassword);
              EXPECT_EQ(state->get_logged_out()->verification->email,
                        kEmailAddress);
            },
            base::Unretained(&pref_service),
            test_case.mojo_expected.has_value())));
  }

  std::string test_name;
  bool seed_verification;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const ResetPasswordVerifyCompleteTestCase*
ResetPasswordVerifyCompleteCalledInWrongState() {
  static const base::NoDestructor<ResetPasswordVerifyCompleteTestCase>
      kResetPasswordVerifyCompleteCalledInWrongState({
          .test_name = "reset_password_verify_complete_called_in_wrong_state",
          .seed_verification = false,
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewClientError(
                  mojom::ResetPasswordClientError::New(
                      mojom::ResetPasswordClientErrorCode::
                          kCalledInWrongState))),
      });
  return kResetPasswordVerifyCompleteCalledInWrongState.get();
}

const ResetPasswordVerifyCompleteTestCase*
ResetPasswordVerifyCompleteVerificationTokenDecryptionFailed() {
  static const base::NoDestructor<ResetPasswordVerifyCompleteTestCase>
      kResetPasswordVerifyCompleteVerificationTokenDecryptionFailed({
          .test_name = "reset_password_verify_complete_verification_token_"
                       "decryption_failed",
          .seed_verification = true,
          .fail_decryption = true,
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewClientError(
                  mojom::ResetPasswordClientError::New(
                      mojom::ResetPasswordClientErrorCode::
                          kVerificationTokenDecryptionFailed))),
      });
  return kResetPasswordVerifyCompleteVerificationTokenDecryptionFailed.get();
}

const ResetPasswordVerifyCompleteTestCase*
ResetPasswordVerifyCompleteBodyMissingOrFailedToParse() {
  static const base::NoDestructor<ResetPasswordVerifyCompleteTestCase>
      kResetPasswordVerifyCompleteBodyMissingOrFailedToParse({
          .test_name =
              "reset_password_verify_complete_body_missing_or_failed_to_parse",
          .seed_verification = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_INTERNAL_SERVER_ERROR,
                      mojom::ResetPasswordServerErrorCode::kInvalidResponse))),
      });
  return kResetPasswordVerifyCompleteBodyMissingOrFailedToParse.get();
}

const ResetPasswordVerifyCompleteTestCase*
ResetPasswordVerifyCompleteErrorCodeIsNull() {
  static const base::NoDestructor<ResetPasswordVerifyCompleteTestCase>
      kResetPasswordVerifyCompleteErrorCodeIsNull({
          .test_name = "reset_password_verify_complete_error_code_is_null",
          .seed_verification = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   VerifyComplete::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_UNAUTHORIZED,
                      mojom::ResetPasswordServerErrorCode::kNull))),
      });
  return kResetPasswordVerifyCompleteErrorCodeIsNull.get();
}

const ResetPasswordVerifyCompleteTestCase*
ResetPasswordVerifyCompleteUnknownErrorCode() {
  static const base::NoDestructor<ResetPasswordVerifyCompleteTestCase>
      kResetPasswordVerifyCompleteUnknownErrorCode({
          .test_name = "reset_password_verify_complete_unknown_error_code",
          .seed_verification = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_TOO_EARLY,
                                 .body = base::unexpected([] {
                                   VerifyComplete::Response::ErrorBody body;
                                   body.code = base::Value(42);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_TOO_EARLY,
                      mojom::ResetPasswordServerErrorCode::kUnknown))),
      });
  return kResetPasswordVerifyCompleteUnknownErrorCode.get();
}

const ResetPasswordVerifyCompleteTestCase*
ResetPasswordVerifyCompleteKnownErrorCode() {
  static const base::NoDestructor<ResetPasswordVerifyCompleteTestCase>
      kResetPasswordVerifyCompleteKnownErrorCode({
          .test_name = "reset_password_verify_complete_known_error_code",
          .seed_verification = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   VerifyComplete::Response::ErrorBody body;
                                   body.code = base::Value(13011);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::ResetPasswordServerErrorCode::
                          kInvalidVerificationCode))),
      });
  return kResetPasswordVerifyCompleteKnownErrorCode.get();
}

const ResetPasswordVerifyCompleteTestCase*
ResetPasswordVerifyCompleteEmailEmpty() {
  static const base::NoDestructor<ResetPasswordVerifyCompleteTestCase>
      kResetPasswordVerifyCompleteEmailEmpty({
          .test_name = "reset_password_verify_complete_email_empty",
          .seed_verification = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       VerifyComplete::Response::SuccessBody
                                           body;
                                       body.auth_token = base::Value();
                                       body.email = "";
                                       return body;
                                     }()}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_OK,
                      mojom::ResetPasswordServerErrorCode::kInvalidResponse))),
      });
  return kResetPasswordVerifyCompleteEmailEmpty.get();
}

const ResetPasswordVerifyCompleteTestCase*
ResetPasswordVerifyCompleteSuccess() {
  static const base::NoDestructor<ResetPasswordVerifyCompleteTestCase>
      kResetPasswordVerifyCompleteSuccess({
          .test_name = "reset_password_verify_complete_success",
          .seed_verification = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       VerifyComplete::Response::SuccessBody
                                           body;
                                       body.auth_token = base::Value();
                                       body.email = kEmailAddress;
                                       return body;
                                     }()}},
          .mojo_expected = mojom::ResetPasswordVerifyCompleteResult::New(),
      });
  return kResetPasswordVerifyCompleteSuccess.get();
}

using BraveAccountServiceResetPasswordVerifyCompleteTest =
    BraveAccountServiceTest<ResetPasswordVerifyCompleteTestCase>;

}  // namespace

TEST_P(BraveAccountServiceResetPasswordVerifyCompleteTest,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceResetPasswordVerifyCompleteTest,
    testing::Values(
        ResetPasswordVerifyCompleteCalledInWrongState(),
        ResetPasswordVerifyCompleteVerificationTokenDecryptionFailed(),
        ResetPasswordVerifyCompleteBodyMissingOrFailedToParse(),
        ResetPasswordVerifyCompleteErrorCodeIsNull(),
        ResetPasswordVerifyCompleteUnknownErrorCode(),
        ResetPasswordVerifyCompleteKnownErrorCode(),
        ResetPasswordVerifyCompleteEmailEmpty(),
        ResetPasswordVerifyCompleteSuccess()),
    BraveAccountServiceResetPasswordVerifyCompleteTest::kNameGenerator);

struct ResetPasswordPasswordInitTestCase {
  using Endpoint = PasswordInit;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::ResetPasswordPasswordInitResultPtr,
                                      mojom::ResetPasswordErrorPtr>;

  static void Run(const ResetPasswordPasswordInitTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    if (test_case.seed_verification) {
      SeedResetPasswordVerification(pref_service);
    }

    authentication->ResetPasswordPasswordInit(
        mojom::Service::kAccounts, "blinded_message", std::move(callback));
  }

  std::string test_name;
  bool seed_verification;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const ResetPasswordPasswordInitTestCase*
ResetPasswordPasswordInitCalledInWrongState() {
  static const base::NoDestructor<ResetPasswordPasswordInitTestCase>
      kResetPasswordPasswordInitCalledInWrongState({
          .test_name = "reset_password_password_init_called_in_wrong_state",
          .seed_verification = false,
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewClientError(
                  mojom::ResetPasswordClientError::New(
                      mojom::ResetPasswordClientErrorCode::
                          kCalledInWrongState))),
      });
  return kResetPasswordPasswordInitCalledInWrongState.get();
}

const ResetPasswordPasswordInitTestCase*
ResetPasswordPasswordInitVerificationTokenDecryptionFailed() {
  static const base::NoDestructor<ResetPasswordPasswordInitTestCase>
      kResetPasswordPasswordInitVerificationTokenDecryptionFailed({
          .test_name = "reset_password_password_init_verification_token_"
                       "decryption_failed",
          .seed_verification = true,
          .fail_decryption = true,
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewClientError(
                  mojom::ResetPasswordClientError::New(
                      mojom::ResetPasswordClientErrorCode::
                          kVerificationTokenDecryptionFailed))),
      });
  return kResetPasswordPasswordInitVerificationTokenDecryptionFailed.get();
}

const ResetPasswordPasswordInitTestCase*
ResetPasswordPasswordInitBodyMissingOrFailedToParse() {
  static const base::NoDestructor<ResetPasswordPasswordInitTestCase>
      kResetPasswordPasswordInitBodyMissingOrFailedToParse({
          .test_name =
              "reset_password_password_init_body_missing_or_failed_to_parse",
          .seed_verification = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_INTERNAL_SERVER_ERROR,
                      mojom::ResetPasswordServerErrorCode::kInvalidResponse))),
      });
  return kResetPasswordPasswordInitBodyMissingOrFailedToParse.get();
}

const ResetPasswordPasswordInitTestCase*
ResetPasswordPasswordInitErrorCodeIsNull() {
  static const base::NoDestructor<ResetPasswordPasswordInitTestCase>
      kResetPasswordPasswordInitErrorCodeIsNull({
          .test_name = "reset_password_password_init_error_code_is_null",
          .seed_verification = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   PasswordInit::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::ResetPasswordServerErrorCode::kNull))),
      });
  return kResetPasswordPasswordInitErrorCodeIsNull.get();
}

const ResetPasswordPasswordInitTestCase*
ResetPasswordPasswordInitUnknownErrorCode() {
  static const base::NoDestructor<ResetPasswordPasswordInitTestCase>
      kResetPasswordPasswordInitUnknownErrorCode({
          .test_name = "reset_password_password_init_unknown_error_code",
          .seed_verification = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_TOO_EARLY,
                                 .body = base::unexpected([] {
                                   PasswordInit::Response::ErrorBody body;
                                   body.code = base::Value(42);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_TOO_EARLY,
                      mojom::ResetPasswordServerErrorCode::kUnknown))),
      });
  return kResetPasswordPasswordInitUnknownErrorCode.get();
}

const ResetPasswordPasswordInitTestCase*
ResetPasswordPasswordInitKnownErrorCode() {
  static const base::NoDestructor<ResetPasswordPasswordInitTestCase>
      kResetPasswordPasswordInitKnownErrorCode({
          .test_name = "reset_password_password_init_known_error_code",
          .seed_verification = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   PasswordInit::Response::ErrorBody body;
                                   body.code = base::Value(13004);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::ResetPasswordServerErrorCode::kAccountExists))),
      });
  return kResetPasswordPasswordInitKnownErrorCode.get();
}

const ResetPasswordPasswordInitTestCase*
ResetPasswordPasswordInitSerializedResponseEmpty() {
  static const base::NoDestructor<ResetPasswordPasswordInitTestCase>
      kResetPasswordPasswordInitSerializedResponseEmpty({
          .test_name = "reset_password_password_init_serialized_response_empty",
          .seed_verification = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       PasswordInit::Response::SuccessBody body;
                                       body.serialized_response = "";
                                       return body;
                                     }()}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_OK,
                      mojom::ResetPasswordServerErrorCode::kInvalidResponse))),
      });
  return kResetPasswordPasswordInitSerializedResponseEmpty.get();
}

const ResetPasswordPasswordInitTestCase* ResetPasswordPasswordInitSuccess() {
  static const base::NoDestructor<ResetPasswordPasswordInitTestCase>
      kResetPasswordPasswordInitSuccess({
          .test_name = "reset_password_password_init_success",
          .seed_verification = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       PasswordInit::Response::SuccessBody body;
                                       body.serialized_response =
                                           "serialized_response";
                                       return body;
                                     }()}},
          .mojo_expected = mojom::ResetPasswordPasswordInitResult::New(
              "serialized_response"),
      });
  return kResetPasswordPasswordInitSuccess.get();
}

using BraveAccountServiceResetPasswordPasswordInitTest =
    BraveAccountServiceTest<ResetPasswordPasswordInitTestCase>;

}  // namespace

TEST_P(BraveAccountServiceResetPasswordPasswordInitTest,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceResetPasswordPasswordInitTest,
    testing::Values(
        ResetPasswordPasswordInitCalledInWrongState(),
        ResetPasswordPasswordInitVerificationTokenDecryptionFailed(),
        ResetPasswordPasswordInitBodyMissingOrFailedToParse(),
        ResetPasswordPasswordInitErrorCodeIsNull(),
        ResetPasswordPasswordInitUnknownErrorCode(),
        ResetPasswordPasswordInitKnownErrorCode(),
        ResetPasswordPasswordInitSerializedResponseEmpty(),
        ResetPasswordPasswordInitSuccess()),
    BraveAccountServiceResetPasswordPasswordInitTest::kNameGenerator);

struct ResetPasswordPasswordFinalizeTestCase {
  using Endpoint = PasswordFinalize;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected =
      base::expected<mojom::ResetPasswordPasswordFinalizeResultPtr,
                     mojom::ResetPasswordErrorPtr>;

  static void Run(const ResetPasswordPasswordFinalizeTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    if (test_case.seed_verification) {
      SeedResetPasswordVerification(pref_service);
    }

    authentication->ResetPasswordPasswordFinalize(
        "serialized_record", kEmailAddress,
        std::move(callback).Then(base::BindOnce(
            [](PrefService* pref_service, bool success) {
              AccountStatePrefs account_state_prefs(*pref_service);
              const auto state = account_state_prefs.GetAccountState();
              if (!success) {
                EXPECT_TRUE(state->is_logged_out());
              } else {
                ASSERT_TRUE(state->is_logged_in());
                EXPECT_EQ(state->get_logged_in()->email, kEmailAddress);
                EXPECT_EQ(account_state_prefs.GetAuthenticationToken(),
                          EncryptedAuthenticationToken());
              }
            },
            base::Unretained(&pref_service),
            test_case.mojo_expected.has_value())));
  }

  std::string test_name;
  bool seed_verification;
  bool fail_decryption;
  bool fail_encryption;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeCalledInWrongState() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeCalledInWrongState({
          .test_name = "reset_password_password_finalize_called_in_wrong_state",
          .seed_verification = false,
          .fail_decryption = {},    // not used
          .fail_encryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewClientError(
                  mojom::ResetPasswordClientError::New(
                      mojom::ResetPasswordClientErrorCode::
                          kCalledInWrongState))),
      });
  return kResetPasswordPasswordFinalizeCalledInWrongState.get();
}

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeVerificationTokenDecryptionFailed() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeVerificationTokenDecryptionFailed({
          .test_name = "reset_password_password_finalize_verification_token_"
                       "decryption_failed",
          .seed_verification = true,
          .fail_decryption = true,
          .fail_encryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewClientError(
                  mojom::ResetPasswordClientError::New(
                      mojom::ResetPasswordClientErrorCode::
                          kVerificationTokenDecryptionFailed))),
      });
  return kResetPasswordPasswordFinalizeVerificationTokenDecryptionFailed.get();
}

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeBodyMissingOrFailedToParse() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeBodyMissingOrFailedToParse({
          .test_name =
              "reset_password_password_finalize_body_missing_or_failed_to_"
              "parse",
          .seed_verification = true,
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_INTERNAL_SERVER_ERROR,
                      mojom::ResetPasswordServerErrorCode::kInvalidResponse))),
      });
  return kResetPasswordPasswordFinalizeBodyMissingOrFailedToParse.get();
}

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeErrorCodeIsNull() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeErrorCodeIsNull({
          .test_name = "reset_password_password_finalize_error_code_is_null",
          .seed_verification = true,
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_NOT_FOUND,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_NOT_FOUND,
                      mojom::ResetPasswordServerErrorCode::kNull))),
      });
  return kResetPasswordPasswordFinalizeErrorCodeIsNull.get();
}

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeUnknownErrorCode() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeUnknownErrorCode({
          .test_name = "reset_password_password_finalize_unknown_error_code",
          .seed_verification = true,
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_TOO_EARLY,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value(42);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_TOO_EARLY,
                      mojom::ResetPasswordServerErrorCode::kUnknown))),
      });
  return kResetPasswordPasswordFinalizeUnknownErrorCode.get();
}

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeKnownErrorCode() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeKnownErrorCode({
          .test_name = "reset_password_password_finalize_known_error_code",
          .seed_verification = true,
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_NOT_FOUND,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value(14001);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_NOT_FOUND, mojom::ResetPasswordServerErrorCode::
                                               kInterimPasswordStateNotFound))),
      });
  return kResetPasswordPasswordFinalizeKnownErrorCode.get();
}

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeAuthenticationTokenNull() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeAuthenticationTokenNull({
          .test_name =
              "reset_password_password_finalize_authentication_token_null",
          .seed_verification = true,
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       PasswordFinalize::Response::SuccessBody
                                           body;
                                       body.auth_token = base::Value();
                                       return body;
                                     }()}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_OK,
                      mojom::ResetPasswordServerErrorCode::kInvalidResponse))),
      });
  return kResetPasswordPasswordFinalizeAuthenticationTokenNull.get();
}

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeAuthenticationTokenEmpty() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeAuthenticationTokenEmpty({
          .test_name =
              "reset_password_password_finalize_authentication_token_empty",
          .seed_verification = true,
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       PasswordFinalize::Response::SuccessBody
                                           body;
                                       body.auth_token = base::Value("");
                                       return body;
                                     }()}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_OK,
                      mojom::ResetPasswordServerErrorCode::kInvalidResponse))),
      });
  return kResetPasswordPasswordFinalizeAuthenticationTokenEmpty.get();
}

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeAuthenticationTokenEncryptionFailed() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeAuthenticationTokenEncryptionFailed({
          .test_name = "reset_password_password_finalize_authentication_token_"
                       "encryption_"
                       "failed",
          .seed_verification = true,
          .fail_decryption = false,
          .fail_encryption = true,
          .endpoint_response =
              {{.net_error = net::OK,
                .status_code = net::HTTP_OK,
                .body =
                    [] {
                      PasswordFinalize::Response::SuccessBody body;
                      body.auth_token = base::Value(kAuthenticationToken);
                      return body;
                    }()}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewClientError(
                  mojom::ResetPasswordClientError::New(
                      mojom::ResetPasswordClientErrorCode::
                          kAuthenticationTokenEncryptionFailed))),
      });
  return kResetPasswordPasswordFinalizeAuthenticationTokenEncryptionFailed
      .get();
}

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeSuccess() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeSuccess({
          .test_name = "reset_password_password_finalize_success",
          .seed_verification = true,
          .fail_decryption = false,
          .fail_encryption = false,
          .endpoint_response =
              {{.net_error = net::OK,
                .status_code = net::HTTP_OK,
                .body =
                    [] {
                      PasswordFinalize::Response::SuccessBody body;
                      body.auth_token = base::Value(kAuthenticationToken);
                      return body;
                    }()}},
          .mojo_expected = mojom::ResetPasswordPasswordFinalizeResult::New(),
      });
  return kResetPasswordPasswordFinalizeSuccess.get();
}

using BraveAccountServiceResetPasswordPasswordFinalizeTest =
    BraveAccountServiceTest<ResetPasswordPasswordFinalizeTestCase>;

}  // namespace

TEST_P(BraveAccountServiceResetPasswordPasswordFinalizeTest,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceResetPasswordPasswordFinalizeTest,
    testing::Values(
        ResetPasswordPasswordFinalizeCalledInWrongState(),
        ResetPasswordPasswordFinalizeVerificationTokenDecryptionFailed(),
        ResetPasswordPasswordFinalizeBodyMissingOrFailedToParse(),
        ResetPasswordPasswordFinalizeErrorCodeIsNull(),
        ResetPasswordPasswordFinalizeUnknownErrorCode(),
        ResetPasswordPasswordFinalizeKnownErrorCode(),
        ResetPasswordPasswordFinalizeAuthenticationTokenNull(),
        ResetPasswordPasswordFinalizeAuthenticationTokenEmpty(),
        ResetPasswordPasswordFinalizeAuthenticationTokenEncryptionFailed(),
        ResetPasswordPasswordFinalizeSuccess()),
    BraveAccountServiceResetPasswordPasswordFinalizeTest::kNameGenerator);

}  // namespace brave_account
