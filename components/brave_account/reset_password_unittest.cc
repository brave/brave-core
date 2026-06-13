/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/reset_password.h"

#include <optional>
#include <string>

#include "base/base64.h"
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

namespace {

constexpr char kAuthenticationToken[] = "authentication_token";
constexpr char kEmailAddress[] = "email@address.com";
constexpr char kVerificationToken[] = "verification_token";

const std::string& EncryptedAuthenticationToken() {
  static const base::NoDestructor<std::string> kEncryptedAuthenticationToken(
      base::Base64Encode(kAuthenticationToken));
  return *kEncryptedAuthenticationToken;
}

const std::string& EncryptedVerificationToken() {
  static const base::NoDestructor<std::string> kEncryptedVerificationToken(
      base::Base64Encode(kVerificationToken));
  return *kEncryptedVerificationToken;
}

}  // namespace

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

// ResetPasswordVerifyInit -> /v2/verify/init -------------------------------

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
        mojom::Service::kAccounts, test_case.email,
        std::move(callback).Then(base::BindOnce(
            [](PrefService* pref_service, bool expect_verification) {
              AccountStatePrefs account_state_prefs(*pref_service);
              const auto state = account_state_prefs.GetAccountState();
              ASSERT_TRUE(state->is_logged_out());
              if (expect_verification) {
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
  std::string email;
  bool fail_encryption;
  bool fail_decryption;
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
          .email = kEmailAddress,
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
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
          .email = kEmailAddress,
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
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
ResetPasswordVerifyInitAccountDoesNotExist() {
  static const base::NoDestructor<ResetPasswordVerifyInitTestCase>
      kResetPasswordVerifyInitAccountDoesNotExist({
          .test_name = "reset_password_verify_init_account_does_not_exist",
          .email = kEmailAddress,
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
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
  return kResetPasswordVerifyInitAccountDoesNotExist.get();
}

const ResetPasswordVerifyInitTestCase*
ResetPasswordVerifyInitTooManyVerifications() {
  static const base::NoDestructor<ResetPasswordVerifyInitTestCase>
      kResetPasswordVerifyInitTooManyVerifications({
          .test_name = "reset_password_verify_init_too_many_verifications",
          .email = kEmailAddress,
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   VerifyInit::Response::ErrorBody body;
                                   body.code = base::Value(13001);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::ResetPasswordServerErrorCode::
                          kTooManyVerifications))),
      });
  return kResetPasswordVerifyInitTooManyVerifications.get();
}

const ResetPasswordVerifyInitTestCase* ResetPasswordVerifyInitServerError() {
  static const base::NoDestructor<ResetPasswordVerifyInitTestCase>
      kResetPasswordVerifyInitServerError({
          .test_name = "reset_password_verify_init_server_error",
          .email = kEmailAddress,
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = base::unexpected([] {
                                   VerifyInit::Response::ErrorBody body;
                                   body.code = base::Value(0);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_INTERNAL_SERVER_ERROR,
                      mojom::ResetPasswordServerErrorCode::kNull))),
      });
  return kResetPasswordVerifyInitServerError.get();
}

const ResetPasswordVerifyInitTestCase* ResetPasswordVerifyInitUnknown() {
  static const base::NoDestructor<ResetPasswordVerifyInitTestCase>
      kResetPasswordVerifyInitUnknown({
          .test_name = "reset_password_verify_init_unknown",
          .email = kEmailAddress,
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
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
  return kResetPasswordVerifyInitUnknown.get();
}

const ResetPasswordVerifyInitTestCase*
ResetPasswordVerifyInitVerificationTokenEmpty() {
  static const base::NoDestructor<ResetPasswordVerifyInitTestCase>
      kResetPasswordVerifyInitVerificationTokenEmpty({
          .test_name = "reset_password_verify_init_verification_token_empty",
          .email = kEmailAddress,
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
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
ResetPasswordVerifyInitVerificationTokenFailedToEncrypt() {
  static const base::NoDestructor<ResetPasswordVerifyInitTestCase>
      kResetPasswordVerifyInitVerificationTokenFailedToEncrypt({
          .test_name =
              "reset_password_verify_init_verification_token_failed_to_encrypt",
          .email = kEmailAddress,
          .fail_encryption = true,
          .fail_decryption = {},  // not used
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
  return kResetPasswordVerifyInitVerificationTokenFailedToEncrypt.get();
}

const ResetPasswordVerifyInitTestCase* ResetPasswordVerifyInitSuccess() {
  static const base::NoDestructor<ResetPasswordVerifyInitTestCase>
      kResetPasswordVerifyInitSuccess({
          .test_name = "reset_password_verify_init_success",
          .email = kEmailAddress,
          .fail_encryption = false,
          .fail_decryption = {},  // not used
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
                    ResetPasswordVerifyInitAccountDoesNotExist(),
                    ResetPasswordVerifyInitTooManyVerifications(),
                    ResetPasswordVerifyInitServerError(),
                    ResetPasswordVerifyInitUnknown(),
                    ResetPasswordVerifyInitVerificationTokenEmpty(),
                    ResetPasswordVerifyInitVerificationTokenFailedToEncrypt(),
                    ResetPasswordVerifyInitSuccess()),
    BraveAccountServiceResetPasswordVerifyInitTest::kNameGenerator);

// ResetPasswordVerifyComplete -> /v2/verify/complete -----------------------

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
        test_case.code,
        std::move(callback).Then(base::BindOnce(
            [](PrefService* pref_service, std::string expected_email) {
              if (expected_email.empty()) {
                return;
              }
              // On success the verified email is recorded on the pending
              // `kResetPassword` verification slot; the state stays logged out
              // with verification.
              AccountStatePrefs account_state_prefs(*pref_service);
              const auto state = account_state_prefs.GetAccountState();
              ASSERT_TRUE(state->is_logged_out());
              ASSERT_TRUE(state->get_logged_out()->verification);
              EXPECT_EQ(state->get_logged_out()->verification->intent,
                        mojom::LoggedOutVerificationIntent::kResetPassword);
              EXPECT_EQ(state->get_logged_out()->verification->email,
                        expected_email);
            },
            base::Unretained(&pref_service), test_case.expected_email)));
  }

  std::string test_name;
  std::string code;
  bool seed_verification;
  bool fail_encryption;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  std::string expected_email;
  MojoExpected mojo_expected;
};

namespace {

const ResetPasswordVerifyCompleteTestCase*
ResetPasswordVerifyCompleteCalledInWrongState() {
  static const base::NoDestructor<ResetPasswordVerifyCompleteTestCase>
      kResetPasswordVerifyCompleteCalledInWrongState({
          .test_name = "reset_password_verify_complete_called_in_wrong_state",
          .code = "23TZMP",
          .seed_verification = false,
          .fail_encryption = {},    // not used
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .expected_email = "",
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewClientError(
                  mojom::ResetPasswordClientError::New(
                      mojom::ResetPasswordClientErrorCode::
                          kCalledInWrongState))),
      });
  return kResetPasswordVerifyCompleteCalledInWrongState.get();
}

const ResetPasswordVerifyCompleteTestCase*
ResetPasswordVerifyCompleteVerificationTokenFailedToDecrypt() {
  static const base::NoDestructor<ResetPasswordVerifyCompleteTestCase>
      kResetPasswordVerifyCompleteVerificationTokenFailedToDecrypt({
          .test_name =
              "reset_password_verify_complete_verification_token_failed_to_"
              "decrypt",
          .code = "23TZMP",
          .seed_verification = true,
          .fail_encryption = {},  // not used
          .fail_decryption = true,
          .endpoint_response = {},  // not used
          .expected_email = "",
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewClientError(
                  mojom::ResetPasswordClientError::New(
                      mojom::ResetPasswordClientErrorCode::
                          kVerificationTokenDecryptionFailed))),
      });
  return kResetPasswordVerifyCompleteVerificationTokenFailedToDecrypt.get();
}

const ResetPasswordVerifyCompleteTestCase*
ResetPasswordVerifyCompleteBodyMissingOrFailedToParse() {
  static const base::NoDestructor<ResetPasswordVerifyCompleteTestCase>
      kResetPasswordVerifyCompleteBodyMissingOrFailedToParse({
          .test_name =
              "reset_password_verify_complete_body_missing_or_failed_to_parse",
          .code = "23TZMP",
          .seed_verification = true,
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .expected_email = "",
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
          .code = "23TZMP",
          .seed_verification = true,
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   VerifyComplete::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .expected_email = "",
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_UNAUTHORIZED,
                      mojom::ResetPasswordServerErrorCode::kNull))),
      });
  return kResetPasswordVerifyCompleteErrorCodeIsNull.get();
}

const ResetPasswordVerifyCompleteTestCase*
ResetPasswordVerifyCompleteInvalidVerificationCode() {
  static const base::NoDestructor<ResetPasswordVerifyCompleteTestCase>
      kResetPasswordVerifyCompleteInvalidVerificationCode({
          .test_name =
              "reset_password_verify_complete_invalid_verification_code",
          .code = "23TZMP",
          .seed_verification = true,
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   VerifyComplete::Response::ErrorBody body;
                                   body.code = base::Value(13011);
                                   return body;
                                 }())}},
          .expected_email = "",
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::ResetPasswordServerErrorCode::
                          kInvalidVerificationCode))),
      });
  return kResetPasswordVerifyCompleteInvalidVerificationCode.get();
}

const ResetPasswordVerifyCompleteTestCase*
ResetPasswordVerifyCompleteUnknown() {
  static const base::NoDestructor<ResetPasswordVerifyCompleteTestCase>
      kResetPasswordVerifyCompleteUnknown({
          .test_name = "reset_password_verify_complete_unknown",
          .code = "23TZMP",
          .seed_verification = true,
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_TOO_EARLY,
                                 .body = base::unexpected([] {
                                   VerifyComplete::Response::ErrorBody body;
                                   body.code = base::Value(42);
                                   return body;
                                 }())}},
          .expected_email = "",
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_TOO_EARLY,
                      mojom::ResetPasswordServerErrorCode::kUnknown))),
      });
  return kResetPasswordVerifyCompleteUnknown.get();
}

const ResetPasswordVerifyCompleteTestCase*
ResetPasswordVerifyCompleteEmailEmpty() {
  static const base::NoDestructor<ResetPasswordVerifyCompleteTestCase>
      kResetPasswordVerifyCompleteEmailEmpty({
          .test_name = "reset_password_verify_complete_email_empty",
          .code = "23TZMP",
          .seed_verification = true,
          .fail_encryption = {},  // not used
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
          .expected_email = "",
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
          .code = "23TZMP",
          .seed_verification = true,
          .fail_encryption = {},  // not used
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
          .expected_email = kEmailAddress,
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
        ResetPasswordVerifyCompleteVerificationTokenFailedToDecrypt(),
        ResetPasswordVerifyCompleteBodyMissingOrFailedToParse(),
        ResetPasswordVerifyCompleteErrorCodeIsNull(),
        ResetPasswordVerifyCompleteInvalidVerificationCode(),
        ResetPasswordVerifyCompleteUnknown(),
        ResetPasswordVerifyCompleteEmailEmpty(),
        ResetPasswordVerifyCompleteSuccess()),
    BraveAccountServiceResetPasswordVerifyCompleteTest::kNameGenerator);

// ResetPasswordPasswordInit -> /v2/accounts/password/init ------------------

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

    authentication->ResetPasswordPasswordInit(mojom::Service::kAccounts,
                                              test_case.blinded_message,
                                              std::move(callback));
  }

  std::string test_name;
  std::string blinded_message;
  bool seed_verification;
  bool fail_encryption;
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
          .blinded_message = "blinded_message",
          .seed_verification = false,
          .fail_encryption = {},    // not used
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
ResetPasswordPasswordInitVerificationTokenFailedToDecrypt() {
  static const base::NoDestructor<ResetPasswordPasswordInitTestCase>
      kResetPasswordPasswordInitVerificationTokenFailedToDecrypt({
          .test_name =
              "reset_password_password_init_verification_token_failed_to_"
              "decrypt",
          .blinded_message = "blinded_message",
          .seed_verification = true,
          .fail_encryption = {},  // not used
          .fail_decryption = true,
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewClientError(
                  mojom::ResetPasswordClientError::New(
                      mojom::ResetPasswordClientErrorCode::
                          kVerificationTokenDecryptionFailed))),
      });
  return kResetPasswordPasswordInitVerificationTokenFailedToDecrypt.get();
}

const ResetPasswordPasswordInitTestCase*
ResetPasswordPasswordInitBodyMissingOrFailedToParse() {
  static const base::NoDestructor<ResetPasswordPasswordInitTestCase>
      kResetPasswordPasswordInitBodyMissingOrFailedToParse({
          .test_name =
              "reset_password_password_init_body_missing_or_failed_to_parse",
          .blinded_message = "blinded_message",
          .seed_verification = true,
          .fail_encryption = {},  // not used
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
          .blinded_message = "blinded_message",
          .seed_verification = true,
          .fail_encryption = {},  // not used
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
ResetPasswordPasswordInitServerError() {
  static const base::NoDestructor<ResetPasswordPasswordInitTestCase>
      kResetPasswordPasswordInitServerError({
          .test_name = "reset_password_password_init_server_error",
          .blinded_message = "blinded_message",
          .seed_verification = true,
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = base::unexpected([] {
                                   PasswordInit::Response::ErrorBody body;
                                   body.code = base::Value(0);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_INTERNAL_SERVER_ERROR,
                      mojom::ResetPasswordServerErrorCode::kNull))),
      });
  return kResetPasswordPasswordInitServerError.get();
}

const ResetPasswordPasswordInitTestCase* ResetPasswordPasswordInitUnknown() {
  static const base::NoDestructor<ResetPasswordPasswordInitTestCase>
      kResetPasswordPasswordInitUnknown({
          .test_name = "reset_password_password_init_unknown",
          .blinded_message = "blinded_message",
          .seed_verification = true,
          .fail_encryption = {},  // not used
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
  return kResetPasswordPasswordInitUnknown.get();
}

const ResetPasswordPasswordInitTestCase*
ResetPasswordPasswordInitSerializedResponseEmpty() {
  static const base::NoDestructor<ResetPasswordPasswordInitTestCase>
      kResetPasswordPasswordInitSerializedResponseEmpty({
          .test_name = "reset_password_password_init_serialized_response_empty",
          .blinded_message = "blinded_message",
          .seed_verification = true,
          .fail_encryption = {},  // not used
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
          .blinded_message = "blinded_message",
          .seed_verification = true,
          .fail_encryption = {},  // not used
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
    testing::Values(ResetPasswordPasswordInitCalledInWrongState(),
                    ResetPasswordPasswordInitVerificationTokenFailedToDecrypt(),
                    ResetPasswordPasswordInitBodyMissingOrFailedToParse(),
                    ResetPasswordPasswordInitErrorCodeIsNull(),
                    ResetPasswordPasswordInitServerError(),
                    ResetPasswordPasswordInitUnknown(),
                    ResetPasswordPasswordInitSerializedResponseEmpty(),
                    ResetPasswordPasswordInitSuccess()),
    BraveAccountServiceResetPasswordPasswordInitTest::kNameGenerator);

// ResetPasswordPasswordFinalize -> /v2/accounts/password/finalize ----------

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
        test_case.serialized_record, test_case.email,
        std::move(callback).Then(base::BindOnce(
            [](PrefService* pref_service, std::string expected_email,
               std::string expected_authentication_token) {
              AccountStatePrefs account_state_prefs(*pref_service);
              const auto state = account_state_prefs.GetAccountState();
              if (expected_authentication_token.empty()) {
                // On failure the user remains logged out.
                EXPECT_TRUE(state->is_logged_out());
              } else {
                // On success the user is logged in (state swap).
                ASSERT_TRUE(state->is_logged_in());
                EXPECT_EQ(state->get_logged_in()->email, expected_email);
                EXPECT_EQ(account_state_prefs.GetAuthenticationToken(),
                          expected_authentication_token);
              }
            },
            base::Unretained(&pref_service), test_case.email,
            test_case.expected_authentication_token)));
  }

  std::string test_name;
  std::string serialized_record;
  std::string email;
  bool seed_verification;
  bool fail_encryption;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  std::string expected_authentication_token;
  MojoExpected mojo_expected;
};

namespace {

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeCalledInWrongState() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeCalledInWrongState({
          .test_name = "reset_password_password_finalize_called_in_wrong_state",
          .serialized_record = "serialized_record",
          .email = kEmailAddress,
          .seed_verification = false,
          .fail_encryption = {},    // not used
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .expected_authentication_token = "",
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewClientError(
                  mojom::ResetPasswordClientError::New(
                      mojom::ResetPasswordClientErrorCode::
                          kCalledInWrongState))),
      });
  return kResetPasswordPasswordFinalizeCalledInWrongState.get();
}

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeVerificationTokenFailedToDecrypt() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeVerificationTokenFailedToDecrypt({
          .test_name =
              "reset_password_password_finalize_verification_token_failed_to_"
              "decrypt",
          .serialized_record = "serialized_record",
          .email = kEmailAddress,
          .seed_verification = true,
          .fail_encryption = {},  // not used
          .fail_decryption = true,
          .endpoint_response = {},  // not used
          .expected_authentication_token = "",
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewClientError(
                  mojom::ResetPasswordClientError::New(
                      mojom::ResetPasswordClientErrorCode::
                          kVerificationTokenDecryptionFailed))),
      });
  return kResetPasswordPasswordFinalizeVerificationTokenFailedToDecrypt.get();
}

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeBodyMissingOrFailedToParse() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeBodyMissingOrFailedToParse({
          .test_name =
              "reset_password_password_finalize_body_missing_or_failed_to_"
              "parse",
          .serialized_record = "serialized_record",
          .email = kEmailAddress,
          .seed_verification = true,
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .expected_authentication_token = "",
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
          .serialized_record = "serialized_record",
          .email = kEmailAddress,
          .seed_verification = true,
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_NOT_FOUND,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .expected_authentication_token = "",
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_NOT_FOUND,
                      mojom::ResetPasswordServerErrorCode::kNull))),
      });
  return kResetPasswordPasswordFinalizeErrorCodeIsNull.get();
}

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeInterimPasswordStateNotFound() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeInterimPasswordStateNotFound({
          .test_name =
              "reset_password_password_finalize_interim_password_state_not_"
              "found",
          .serialized_record = "serialized_record",
          .email = kEmailAddress,
          .seed_verification = true,
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_NOT_FOUND,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value(14001);
                                   return body;
                                 }())}},
          .expected_authentication_token = "",
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_NOT_FOUND, mojom::ResetPasswordServerErrorCode::
                                               kInterimPasswordStateNotFound))),
      });
  return kResetPasswordPasswordFinalizeInterimPasswordStateNotFound.get();
}

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeInterimPasswordStateExpired() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeInterimPasswordStateExpired({
          .test_name =
              "reset_password_password_finalize_interim_password_state_expired",
          .serialized_record = "serialized_record",
          .email = kEmailAddress,
          .seed_verification = true,
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value(14002);
                                   return body;
                                 }())}},
          .expected_authentication_token = "",
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::ResetPasswordServerErrorCode::
                          kInterimPasswordStateExpired))),
      });
  return kResetPasswordPasswordFinalizeInterimPasswordStateExpired.get();
}

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeUnknown() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeUnknown({
          .test_name = "reset_password_password_finalize_unknown",
          .serialized_record = "serialized_record",
          .email = kEmailAddress,
          .seed_verification = true,
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_TOO_EARLY,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value(42);
                                   return body;
                                 }())}},
          .expected_authentication_token = "",
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_TOO_EARLY,
                      mojom::ResetPasswordServerErrorCode::kUnknown))),
      });
  return kResetPasswordPasswordFinalizeUnknown.get();
}

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeAuthTokenNull() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeAuthTokenNull({
          .test_name = "reset_password_password_finalize_auth_token_null",
          .serialized_record = "serialized_record",
          .email = kEmailAddress,
          .seed_verification = true,
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       PasswordFinalize::Response::SuccessBody
                                           body;
                                       body.auth_token = base::Value();
                                       return body;
                                     }()}},
          .expected_authentication_token = "",
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_OK,
                      mojom::ResetPasswordServerErrorCode::kInvalidResponse))),
      });
  return kResetPasswordPasswordFinalizeAuthTokenNull.get();
}

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeAuthTokenEmpty() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeAuthTokenEmpty({
          .test_name = "reset_password_password_finalize_auth_token_empty",
          .serialized_record = "serialized_record",
          .email = kEmailAddress,
          .seed_verification = true,
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       PasswordFinalize::Response::SuccessBody
                                           body;
                                       body.auth_token = base::Value("");
                                       return body;
                                     }()}},
          .expected_authentication_token = "",
          .mojo_expected =
              base::unexpected(mojom::ResetPasswordError::NewServerError(
                  mojom::ResetPasswordServerError::New(
                      net::HTTP_OK,
                      mojom::ResetPasswordServerErrorCode::kInvalidResponse))),
      });
  return kResetPasswordPasswordFinalizeAuthTokenEmpty.get();
}

const ResetPasswordPasswordFinalizeTestCase*
ResetPasswordPasswordFinalizeAuthenticationTokenEncryptionFailed() {
  static const base::NoDestructor<ResetPasswordPasswordFinalizeTestCase>
      kResetPasswordPasswordFinalizeAuthenticationTokenEncryptionFailed({
          .test_name = "reset_password_password_finalize_authentication_token_"
                       "encryption_"
                       "failed",
          .serialized_record = "serialized_record",
          .email = kEmailAddress,
          .seed_verification = true,
          .fail_encryption = true,
          .fail_decryption = false,
          .endpoint_response =
              {{.net_error = net::OK,
                .status_code = net::HTTP_OK,
                .body =
                    [] {
                      PasswordFinalize::Response::SuccessBody body;
                      body.auth_token = base::Value(kAuthenticationToken);
                      return body;
                    }()}},
          .expected_authentication_token = "",
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
          .serialized_record = "serialized_record",
          .email = kEmailAddress,
          .seed_verification = true,
          .fail_encryption = false,
          .fail_decryption = false,
          .endpoint_response =
              {{.net_error = net::OK,
                .status_code = net::HTTP_OK,
                .body =
                    [] {
                      PasswordFinalize::Response::SuccessBody body;
                      body.auth_token = base::Value(kAuthenticationToken);
                      return body;
                    }()}},
          .expected_authentication_token = EncryptedAuthenticationToken(),
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
        ResetPasswordPasswordFinalizeVerificationTokenFailedToDecrypt(),
        ResetPasswordPasswordFinalizeBodyMissingOrFailedToParse(),
        ResetPasswordPasswordFinalizeErrorCodeIsNull(),
        ResetPasswordPasswordFinalizeInterimPasswordStateNotFound(),
        ResetPasswordPasswordFinalizeInterimPasswordStateExpired(),
        ResetPasswordPasswordFinalizeUnknown(),
        ResetPasswordPasswordFinalizeAuthTokenNull(),
        ResetPasswordPasswordFinalizeAuthTokenEmpty(),
        ResetPasswordPasswordFinalizeAuthenticationTokenEncryptionFailed(),
        ResetPasswordPasswordFinalizeSuccess()),
    BraveAccountServiceResetPasswordPasswordFinalizeTest::kNameGenerator);

}  // namespace brave_account
