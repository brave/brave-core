/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/reset_password.h"

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
#include "brave/components/brave_account/mojom/reset_password.mojom.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

using endpoints::PasswordFinalize;
using endpoints::PasswordInit;
using endpoints::VerifyComplete;
using endpoints::VerifyInit;

// `ResetPasswordStep2`, `ResetPasswordStep3`, and
// `ResetPasswordStep4` all require a pending `kResetPassword`
// verification slot in prefs (the token set by `ResetPasswordStep1`).
// Seed it before invoking the method under test.
namespace {

void SeedResetPasswordVerification(PrefService& pref_service) {
  AccountStatePrefs(pref_service)
      .AddVerification(EncryptedVerificationToken(),
                       mojom::VerificationIntent::NewLoggedOutIntent(
                           mojom::LoggedOutVerificationIntent::kResetPassword));
}

}  // namespace

struct ResetPasswordStep1TestCase {
  using Endpoint = VerifyInit;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::ResetPasswordStep1ResultPtr,
                                      mojom::ResetPasswordErrorPtr>;

  static void Run(const ResetPasswordStep1TestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    authentication->ResetPasswordStep1(
        kEmailAddress,
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

const ResetPasswordStep1TestCase*
ResetPasswordStep1BodyMissingOrFailedToParse() {
  static const base::NoDestructor<ResetPasswordStep1TestCase> kTestCase({
      .test_name = "reset_password_step1_body_missing_or_failed_to_parse",
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
  return kTestCase.get();
}

const ResetPasswordStep1TestCase* ResetPasswordStep1ErrorCodeIsNull() {
  static const base::NoDestructor<ResetPasswordStep1TestCase> kTestCase({
      .test_name = "reset_password_step1_error_code_is_null",
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
  return kTestCase.get();
}

const ResetPasswordStep1TestCase* ResetPasswordStep1UnknownErrorCode() {
  static const base::NoDestructor<ResetPasswordStep1TestCase> kTestCase({
      .test_name = "reset_password_step1_unknown_error_code",
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
  return kTestCase.get();
}

const ResetPasswordStep1TestCase* ResetPasswordStep1KnownErrorCode() {
  static const base::NoDestructor<ResetPasswordStep1TestCase> kTestCase({
      .test_name = "reset_password_step1_known_error_code",
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
                  mojom::ResetPasswordServerErrorCode::kAccountDoesNotExist))),
  });
  return kTestCase.get();
}

const ResetPasswordStep1TestCase* ResetPasswordStep1VerificationTokenEmpty() {
  static const base::NoDestructor<ResetPasswordStep1TestCase> kTestCase({
      .test_name = "reset_password_step1_verification_token_empty",
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
  return kTestCase.get();
}

const ResetPasswordStep1TestCase*
ResetPasswordStep1VerificationTokenEncryptionFailed() {
  static const base::NoDestructor<ResetPasswordStep1TestCase> kTestCase({
      .test_name = "reset_password_step1_verification_token_encryption_failed",
      .fail_encryption = true,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   VerifyInit::Response::SuccessBody body;
                                   body.verification_token = kVerificationToken;
                                   return body;
                                 }()}},
      .mojo_expected =
          base::unexpected(mojom::ResetPasswordError::NewClientError(
              mojom::ResetPasswordClientError::New(
                  mojom::ResetPasswordClientErrorCode::
                      kVerificationTokenEncryptionFailed))),
  });
  return kTestCase.get();
}

const ResetPasswordStep1TestCase* ResetPasswordStep1Success() {
  static const base::NoDestructor<ResetPasswordStep1TestCase> kTestCase({
      .test_name = "reset_password_step1_success",
      .fail_encryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   VerifyInit::Response::SuccessBody body;
                                   body.verification_token = kVerificationToken;
                                   return body;
                                 }()}},
      .mojo_expected = mojom::ResetPasswordStep1Result::New(),
  });
  return kTestCase.get();
}

using BraveAccountServiceResetPasswordStep1Test =
    BraveAccountServiceTest<ResetPasswordStep1TestCase>;

}  // namespace

TEST_P(BraveAccountServiceResetPasswordStep1Test,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceResetPasswordStep1Test,
    testing::Values(ResetPasswordStep1BodyMissingOrFailedToParse(),
                    ResetPasswordStep1ErrorCodeIsNull(),
                    ResetPasswordStep1UnknownErrorCode(),
                    ResetPasswordStep1KnownErrorCode(),
                    ResetPasswordStep1VerificationTokenEmpty(),
                    ResetPasswordStep1VerificationTokenEncryptionFailed(),
                    ResetPasswordStep1Success()),
    BraveAccountServiceResetPasswordStep1Test::kNameGenerator);

struct ResetPasswordStep2TestCase {
  using Endpoint = VerifyComplete;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::ResetPasswordStep2ResultPtr,
                                      mojom::ResetPasswordErrorPtr>;

  static void Run(const ResetPasswordStep2TestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    if (test_case.seed_verification) {
      SeedResetPasswordVerification(pref_service);
    }

    authentication->ResetPasswordStep2(
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
              EXPECT_EQ(state->get_logged_out()->verification->verified_email,
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

const ResetPasswordStep2TestCase* ResetPasswordStep2CalledInWrongState() {
  static const base::NoDestructor<ResetPasswordStep2TestCase> kTestCase({
      .test_name = "reset_password_step2_called_in_wrong_state",
      .seed_verification = false,
      .fail_decryption = {},    // not used
      .endpoint_response = {},  // not used
      .mojo_expected =
          base::unexpected(mojom::ResetPasswordError::NewClientError(
              mojom::ResetPasswordClientError::New(
                  mojom::ResetPasswordClientErrorCode::kCalledInWrongState))),
  });
  return kTestCase.get();
}

const ResetPasswordStep2TestCase*
ResetPasswordStep2VerificationTokenDecryptionFailed() {
  static const base::NoDestructor<ResetPasswordStep2TestCase> kTestCase({
      .test_name = "reset_password_step2_verification_token_"
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
  return kTestCase.get();
}

const ResetPasswordStep2TestCase*
ResetPasswordStep2BodyMissingOrFailedToParse() {
  static const base::NoDestructor<ResetPasswordStep2TestCase> kTestCase({
      .test_name = "reset_password_step2_body_missing_or_failed_to_parse",
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
  return kTestCase.get();
}

const ResetPasswordStep2TestCase* ResetPasswordStep2ErrorCodeIsNull() {
  static const base::NoDestructor<ResetPasswordStep2TestCase> kTestCase({
      .test_name = "reset_password_step2_error_code_is_null",
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
  return kTestCase.get();
}

const ResetPasswordStep2TestCase* ResetPasswordStep2UnknownErrorCode() {
  static const base::NoDestructor<ResetPasswordStep2TestCase> kTestCase({
      .test_name = "reset_password_step2_unknown_error_code",
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
  return kTestCase.get();
}

const ResetPasswordStep2TestCase* ResetPasswordStep2KnownErrorCode() {
  static const base::NoDestructor<ResetPasswordStep2TestCase> kTestCase({
      .test_name = "reset_password_step2_known_error_code",
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
                  net::HTTP_BAD_REQUEST, mojom::ResetPasswordServerErrorCode::
                                             kInvalidVerificationCode))),
  });
  return kTestCase.get();
}

const ResetPasswordStep2TestCase* ResetPasswordStep2EmailEmpty() {
  static const base::NoDestructor<ResetPasswordStep2TestCase> kTestCase({
      .test_name = "reset_password_step2_email_empty",
      .seed_verification = true,
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   VerifyComplete::Response::SuccessBody body;
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
  return kTestCase.get();
}

const ResetPasswordStep2TestCase* ResetPasswordStep2Success() {
  static const base::NoDestructor<ResetPasswordStep2TestCase> kTestCase({
      .test_name = "reset_password_step2_success",
      .seed_verification = true,
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   VerifyComplete::Response::SuccessBody body;
                                   body.auth_token = base::Value();
                                   body.email = kEmailAddress;
                                   return body;
                                 }()}},
      .mojo_expected = mojom::ResetPasswordStep2Result::New(),
  });
  return kTestCase.get();
}

using BraveAccountServiceResetPasswordStep2Test =
    BraveAccountServiceTest<ResetPasswordStep2TestCase>;

}  // namespace

TEST_P(BraveAccountServiceResetPasswordStep2Test,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceResetPasswordStep2Test,
    testing::Values(ResetPasswordStep2CalledInWrongState(),
                    ResetPasswordStep2VerificationTokenDecryptionFailed(),
                    ResetPasswordStep2BodyMissingOrFailedToParse(),
                    ResetPasswordStep2ErrorCodeIsNull(),
                    ResetPasswordStep2UnknownErrorCode(),
                    ResetPasswordStep2KnownErrorCode(),
                    ResetPasswordStep2EmailEmpty(),
                    ResetPasswordStep2Success()),
    BraveAccountServiceResetPasswordStep2Test::kNameGenerator);

struct ResetPasswordStep3TestCase {
  using Endpoint = PasswordInit;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::ResetPasswordStep3ResultPtr,
                                      mojom::ResetPasswordErrorPtr>;

  static void Run(const ResetPasswordStep3TestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    if (test_case.seed_verification) {
      SeedResetPasswordVerification(pref_service);
    }

    authentication->ResetPasswordStep3("blinded_message", std::move(callback));
  }

  std::string test_name;
  bool seed_verification;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const ResetPasswordStep3TestCase* ResetPasswordStep3CalledInWrongState() {
  static const base::NoDestructor<ResetPasswordStep3TestCase> kTestCase({
      .test_name = "reset_password_step3_called_in_wrong_state",
      .seed_verification = false,
      .fail_decryption = {},    // not used
      .endpoint_response = {},  // not used
      .mojo_expected =
          base::unexpected(mojom::ResetPasswordError::NewClientError(
              mojom::ResetPasswordClientError::New(
                  mojom::ResetPasswordClientErrorCode::kCalledInWrongState))),
  });
  return kTestCase.get();
}

const ResetPasswordStep3TestCase*
ResetPasswordStep3VerificationTokenDecryptionFailed() {
  static const base::NoDestructor<ResetPasswordStep3TestCase> kTestCase({
      .test_name = "reset_password_step3_verification_token_"
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
  return kTestCase.get();
}

const ResetPasswordStep3TestCase*
ResetPasswordStep3BodyMissingOrFailedToParse() {
  static const base::NoDestructor<ResetPasswordStep3TestCase> kTestCase({
      .test_name = "reset_password_step3_body_missing_or_failed_to_parse",
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
  return kTestCase.get();
}

const ResetPasswordStep3TestCase* ResetPasswordStep3ErrorCodeIsNull() {
  static const base::NoDestructor<ResetPasswordStep3TestCase> kTestCase({
      .test_name = "reset_password_step3_error_code_is_null",
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
  return kTestCase.get();
}

const ResetPasswordStep3TestCase* ResetPasswordStep3UnknownErrorCode() {
  static const base::NoDestructor<ResetPasswordStep3TestCase> kTestCase({
      .test_name = "reset_password_step3_unknown_error_code",
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
  return kTestCase.get();
}

const ResetPasswordStep3TestCase* ResetPasswordStep3KnownErrorCode() {
  static const base::NoDestructor<ResetPasswordStep3TestCase> kTestCase({
      .test_name = "reset_password_step3_known_error_code",
      .seed_verification = true,
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_BAD_REQUEST,
                             .body = base::unexpected([] {
                               PasswordInit::Response::ErrorBody body;
                               body.code = base::Value(13001);
                               return body;
                             }())}},
      .mojo_expected =
          base::unexpected(mojom::ResetPasswordError::NewServerError(
              mojom::ResetPasswordServerError::New(
                  net::HTTP_BAD_REQUEST,
                  mojom::ResetPasswordServerErrorCode::kTooManyVerifications))),
  });
  return kTestCase.get();
}

const ResetPasswordStep3TestCase* ResetPasswordStep3SerializedResponseEmpty() {
  static const base::NoDestructor<ResetPasswordStep3TestCase> kTestCase({
      .test_name = "reset_password_step3_serialized_response_empty",
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
  return kTestCase.get();
}

const ResetPasswordStep3TestCase* ResetPasswordStep3Success() {
  static const base::NoDestructor<ResetPasswordStep3TestCase> kTestCase({
      .test_name = "reset_password_step3_success",
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
      .mojo_expected =
          mojom::ResetPasswordStep3Result::New("serialized_response"),
  });
  return kTestCase.get();
}

using BraveAccountServiceResetPasswordStep3Test =
    BraveAccountServiceTest<ResetPasswordStep3TestCase>;

}  // namespace

TEST_P(BraveAccountServiceResetPasswordStep3Test,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceResetPasswordStep3Test,
    testing::Values(ResetPasswordStep3CalledInWrongState(),
                    ResetPasswordStep3VerificationTokenDecryptionFailed(),
                    ResetPasswordStep3BodyMissingOrFailedToParse(),
                    ResetPasswordStep3ErrorCodeIsNull(),
                    ResetPasswordStep3UnknownErrorCode(),
                    ResetPasswordStep3KnownErrorCode(),
                    ResetPasswordStep3SerializedResponseEmpty(),
                    ResetPasswordStep3Success()),
    BraveAccountServiceResetPasswordStep3Test::kNameGenerator);

struct ResetPasswordStep4TestCase {
  using Endpoint = PasswordFinalize;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::ResetPasswordStep4ResultPtr,
                                      mojom::ResetPasswordErrorPtr>;

  static void Run(const ResetPasswordStep4TestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    if (test_case.seed_verification) {
      SeedResetPasswordVerification(pref_service);
    }

    authentication->ResetPasswordStep4(
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

const ResetPasswordStep4TestCase* ResetPasswordStep4CalledInWrongState() {
  static const base::NoDestructor<ResetPasswordStep4TestCase> kTestCase({
      .test_name = "reset_password_step4_called_in_wrong_state",
      .seed_verification = false,
      .fail_decryption = {},    // not used
      .fail_encryption = {},    // not used
      .endpoint_response = {},  // not used
      .mojo_expected =
          base::unexpected(mojom::ResetPasswordError::NewClientError(
              mojom::ResetPasswordClientError::New(
                  mojom::ResetPasswordClientErrorCode::kCalledInWrongState))),
  });
  return kTestCase.get();
}

const ResetPasswordStep4TestCase*
ResetPasswordStep4VerificationTokenDecryptionFailed() {
  static const base::NoDestructor<ResetPasswordStep4TestCase> kTestCase({
      .test_name = "reset_password_step4_verification_token_"
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
  return kTestCase.get();
}

const ResetPasswordStep4TestCase*
ResetPasswordStep4BodyMissingOrFailedToParse() {
  static const base::NoDestructor<ResetPasswordStep4TestCase> kTestCase({
      .test_name = "reset_password_step4_body_missing_or_failed_to_"
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
  return kTestCase.get();
}

const ResetPasswordStep4TestCase* ResetPasswordStep4ErrorCodeIsNull() {
  static const base::NoDestructor<ResetPasswordStep4TestCase> kTestCase({
      .test_name = "reset_password_step4_error_code_is_null",
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
  return kTestCase.get();
}

const ResetPasswordStep4TestCase* ResetPasswordStep4UnknownErrorCode() {
  static const base::NoDestructor<ResetPasswordStep4TestCase> kTestCase({
      .test_name = "reset_password_step4_unknown_error_code",
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
  return kTestCase.get();
}

const ResetPasswordStep4TestCase* ResetPasswordStep4KnownErrorCode() {
  static const base::NoDestructor<ResetPasswordStep4TestCase> kTestCase({
      .test_name = "reset_password_step4_known_error_code",
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
  return kTestCase.get();
}

const ResetPasswordStep4TestCase* ResetPasswordStep4AuthenticationTokenNull() {
  static const base::NoDestructor<ResetPasswordStep4TestCase> kTestCase({
      .test_name = "reset_password_step4_authentication_token_null",
      .seed_verification = true,
      .fail_decryption = false,
      .fail_encryption = {},  // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   PasswordFinalize::Response::SuccessBody body;
                                   body.auth_token = base::Value();
                                   return body;
                                 }()}},
      .mojo_expected =
          base::unexpected(mojom::ResetPasswordError::NewServerError(
              mojom::ResetPasswordServerError::New(
                  net::HTTP_OK,
                  mojom::ResetPasswordServerErrorCode::kInvalidResponse))),
  });
  return kTestCase.get();
}

const ResetPasswordStep4TestCase* ResetPasswordStep4AuthenticationTokenEmpty() {
  static const base::NoDestructor<ResetPasswordStep4TestCase> kTestCase({
      .test_name = "reset_password_step4_authentication_token_empty",
      .seed_verification = true,
      .fail_decryption = false,
      .fail_encryption = {},  // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   PasswordFinalize::Response::SuccessBody body;
                                   body.auth_token = base::Value("");
                                   return body;
                                 }()}},
      .mojo_expected =
          base::unexpected(mojom::ResetPasswordError::NewServerError(
              mojom::ResetPasswordServerError::New(
                  net::HTTP_OK,
                  mojom::ResetPasswordServerErrorCode::kInvalidResponse))),
  });
  return kTestCase.get();
}

const ResetPasswordStep4TestCase*
ResetPasswordStep4AuthenticationTokenEncryptionFailed() {
  static const base::NoDestructor<ResetPasswordStep4TestCase> kTestCase({
      .test_name = "reset_password_step4_authentication_token_"
                   "encryption_"
                   "failed",
      .seed_verification = true,
      .fail_decryption = false,
      .fail_encryption = true,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   PasswordFinalize::Response::SuccessBody body;
                                   body.auth_token =
                                       base::Value(kAuthenticationToken);
                                   return body;
                                 }()}},
      .mojo_expected =
          base::unexpected(mojom::ResetPasswordError::NewClientError(
              mojom::ResetPasswordClientError::New(
                  mojom::ResetPasswordClientErrorCode::
                      kAuthenticationTokenEncryptionFailed))),
  });
  return kTestCase.get();
}

const ResetPasswordStep4TestCase* ResetPasswordStep4Success() {
  static const base::NoDestructor<ResetPasswordStep4TestCase> kTestCase({
      .test_name = "reset_password_step4_success",
      .seed_verification = true,
      .fail_decryption = false,
      .fail_encryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   PasswordFinalize::Response::SuccessBody body;
                                   body.auth_token =
                                       base::Value(kAuthenticationToken);
                                   return body;
                                 }()}},
      .mojo_expected = mojom::ResetPasswordStep4Result::New(),
  });
  return kTestCase.get();
}

using BraveAccountServiceResetPasswordStep4Test =
    BraveAccountServiceTest<ResetPasswordStep4TestCase>;

}  // namespace

TEST_P(BraveAccountServiceResetPasswordStep4Test,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceResetPasswordStep4Test,
    testing::Values(ResetPasswordStep4CalledInWrongState(),
                    ResetPasswordStep4VerificationTokenDecryptionFailed(),
                    ResetPasswordStep4BodyMissingOrFailedToParse(),
                    ResetPasswordStep4ErrorCodeIsNull(),
                    ResetPasswordStep4UnknownErrorCode(),
                    ResetPasswordStep4KnownErrorCode(),
                    ResetPasswordStep4AuthenticationTokenNull(),
                    ResetPasswordStep4AuthenticationTokenEmpty(),
                    ResetPasswordStep4AuthenticationTokenEncryptionFailed(),
                    ResetPasswordStep4Success()),
    BraveAccountServiceResetPasswordStep4Test::kNameGenerator);

}  // namespace brave_account
