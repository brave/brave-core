/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/register.h"

#include <optional>
#include <string>

#include "base/check_deref.h"
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
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

using endpoints::PasswordFinalize;
using endpoints::PasswordInit;
using endpoints::VerifyComplete;

struct RegisterPasswordInitTestCase {
  using Endpoint = PasswordInit;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::RegisterPasswordInitResultPtr,
                                      mojom::RegisterErrorPtr>;

  static void Run(const RegisterPasswordInitTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    authentication->RegisterPasswordInit(
        mojom::Service::kAccounts, test_case.email, test_case.blinded_message,
        std::move(callback));
  }

  std::string test_name;
  std::string email;
  std::string blinded_message;
  bool fail_encryption;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const RegisterPasswordInitTestCase*
RegisterPasswordInitBodyMissingOrFailedToParse() {
  static const base::NoDestructor<RegisterPasswordInitTestCase>
      kRegisterPasswordInitBodyMissingOrFailedToParse({
          .test_name = "register_password_init_body_missing_or_failed_to_parse",
          .email = kEmailAddress,
          .blinded_message = "blinded_message",
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewServerError(
                  mojom::RegisterServerError::New(
                      net::HTTP_INTERNAL_SERVER_ERROR,
                      mojom::RegisterServerErrorCode::kInvalidResponse))),
      });
  return kRegisterPasswordInitBodyMissingOrFailedToParse.get();
}

const RegisterPasswordInitTestCase* RegisterPasswordInitErrorCodeIsNull() {
  static const base::NoDestructor<RegisterPasswordInitTestCase>
      kRegisterPasswordInitErrorCodeIsNull({
          .test_name = "register_password_init_error_code_is_null",
          .email = kEmailAddress,
          .blinded_message = "blinded_message",
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   PasswordInit::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewServerError(
                  mojom::RegisterServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::RegisterServerErrorCode::kNull))),
      });
  return kRegisterPasswordInitErrorCodeIsNull.get();
}

const RegisterPasswordInitTestCase* RegisterPasswordInitUnknownErrorCode() {
  static const base::NoDestructor<RegisterPasswordInitTestCase>
      kRegisterPasswordInitUnknownErrorCode({
          .test_name = "register_password_init_unknown_error_code",
          .email = kEmailAddress,
          .blinded_message = "blinded_message",
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_TOO_EARLY,
                                 .body = base::unexpected([] {
                                   PasswordInit::Response::ErrorBody body;
                                   body.code = base::Value(42);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewServerError(
                  mojom::RegisterServerError::New(
                      net::HTTP_TOO_EARLY,
                      mojom::RegisterServerErrorCode::kUnknown))),
      });
  return kRegisterPasswordInitUnknownErrorCode.get();
}

const RegisterPasswordInitTestCase* RegisterPasswordInitKnownErrorCode() {
  static const base::NoDestructor<RegisterPasswordInitTestCase>
      kRegisterPasswordInitKnownErrorCode({
          .test_name = "register_password_init_known_error_code",
          .email = kEmailAddress,
          .blinded_message = "blinded_message",
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   PasswordInit::Response::ErrorBody body;
                                   body.code = base::Value(11005);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewServerError(
                  mojom::RegisterServerError::New(
                      net::HTTP_BAD_REQUEST, mojom::RegisterServerErrorCode::
                                                 kNewAccountEmailRequired))),
      });
  return kRegisterPasswordInitKnownErrorCode.get();
}

const RegisterPasswordInitTestCase*
RegisterPasswordInitVerificationTokenMissing() {
  static const base::NoDestructor<RegisterPasswordInitTestCase>
      kRegisterPasswordInitVerificationTokenMissing({
          .test_name = "register_password_init_verification_token_missing",
          .email = kEmailAddress,
          .blinded_message = "blinded_message",
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       PasswordInit::Response::SuccessBody body;
                                       body.verification_token = std::nullopt;
                                       body.serialized_response =
                                           "serialized_response";
                                       return body;
                                     }()}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewServerError(
                  mojom::RegisterServerError::New(
                      net::HTTP_OK,
                      mojom::RegisterServerErrorCode::kInvalidResponse))),
      });
  return kRegisterPasswordInitVerificationTokenMissing.get();
}

const RegisterPasswordInitTestCase*
RegisterPasswordInitVerificationTokenEmpty() {
  static const base::NoDestructor<RegisterPasswordInitTestCase>
      kRegisterPasswordInitVerificationTokenEmpty({
          .test_name = "register_password_init_verification_token_empty",
          .email = kEmailAddress,
          .blinded_message = "blinded_message",
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       PasswordInit::Response::SuccessBody body;
                                       body.verification_token = "";
                                       body.serialized_response =
                                           "serialized_response";
                                       return body;
                                     }()}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewServerError(
                  mojom::RegisterServerError::New(
                      net::HTTP_OK,
                      mojom::RegisterServerErrorCode::kInvalidResponse))),
      });
  return kRegisterPasswordInitVerificationTokenEmpty.get();
}

const RegisterPasswordInitTestCase*
RegisterPasswordInitSerializedResponseEmpty() {
  static const base::NoDestructor<RegisterPasswordInitTestCase>
      kRegisterPasswordInitSerializedResponseEmpty({
          .test_name = "register_password_init_serialized_response_empty",
          .email = kEmailAddress,
          .blinded_message = "blinded_message",
          .fail_encryption = {},  // not used
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       PasswordInit::Response::SuccessBody body;
                                       body.verification_token =
                                           kVerificationToken;
                                       body.serialized_response = "";
                                       return body;
                                     }()}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewServerError(
                  mojom::RegisterServerError::New(
                      net::HTTP_OK,
                      mojom::RegisterServerErrorCode::kInvalidResponse))),
      });
  return kRegisterPasswordInitSerializedResponseEmpty.get();
}

const RegisterPasswordInitTestCase*
RegisterPasswordInitVerificationTokenEncryptionFailed() {
  static const base::NoDestructor<RegisterPasswordInitTestCase>
      kRegisterPasswordInitVerificationTokenEncryptionFailed({
          .test_name =
              "register_password_init_verification_token_encryption_failed",
          .email = kEmailAddress,
          .blinded_message = "blinded_message",
          .fail_encryption = true,
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       PasswordInit::Response::SuccessBody body;
                                       body.verification_token =
                                           kVerificationToken;
                                       body.serialized_response =
                                           "serialized_response";
                                       return body;
                                     }()}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewClientError(
                  mojom::RegisterClientError::New(
                      mojom::RegisterClientErrorCode::
                          kVerificationTokenEncryptionFailed))),
      });
  return kRegisterPasswordInitVerificationTokenEncryptionFailed.get();
}

const RegisterPasswordInitTestCase* RegisterPasswordInitSuccess() {
  static const base::NoDestructor<RegisterPasswordInitTestCase>
      kRegisterPasswordInitSuccess({
          .test_name = "register_password_init_success",
          .email = kEmailAddress,
          .blinded_message = "blinded_message",
          .fail_encryption = false,
          .fail_decryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       PasswordInit::Response::SuccessBody body;
                                       body.verification_token =
                                           kVerificationToken;
                                       body.serialized_response =
                                           "serialized_response";
                                       return body;
                                     }()}},
          .mojo_expected = mojom::RegisterPasswordInitResult::New(
              EncryptedVerificationToken(), "serialized_response"),
      });
  return kRegisterPasswordInitSuccess.get();
}

using BraveAccountServiceRegisterPasswordInitTest =
    BraveAccountServiceTest<RegisterPasswordInitTestCase>;

}  // namespace

TEST_P(BraveAccountServiceRegisterPasswordInitTest,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceRegisterPasswordInitTest,
    testing::Values(RegisterPasswordInitBodyMissingOrFailedToParse(),
                    RegisterPasswordInitErrorCodeIsNull(),
                    RegisterPasswordInitUnknownErrorCode(),
                    RegisterPasswordInitKnownErrorCode(),
                    RegisterPasswordInitVerificationTokenMissing(),
                    RegisterPasswordInitVerificationTokenEmpty(),
                    RegisterPasswordInitSerializedResponseEmpty(),
                    RegisterPasswordInitVerificationTokenEncryptionFailed(),
                    RegisterPasswordInitSuccess()),
    BraveAccountServiceRegisterPasswordInitTest::kNameGenerator);

struct RegisterPasswordFinalizeTestCase {
  using Endpoint = PasswordFinalize;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::RegisterPasswordFinalizeResultPtr,
                                      mojom::RegisterErrorPtr>;

  static void Run(const RegisterPasswordFinalizeTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    authentication->RegisterPasswordFinalize(
        test_case.encrypted_verification_token, test_case.serialized_record,
        std::move(callback));
  }

  std::string test_name;
  std::string encrypted_verification_token;
  std::string serialized_record;
  bool fail_encryption;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const RegisterPasswordFinalizeTestCase*
RegisterPasswordFinalizeVerificationTokenDecryptionFailed() {
  static const base::NoDestructor<RegisterPasswordFinalizeTestCase>
      kRegisterPasswordFinalizeVerificationTokenDecryptionFailed({
          .test_name =
              "register_password_finalize_verification_token_decryption_failed",
          .encrypted_verification_token = EncryptedVerificationToken(),
          .serialized_record = "serialized_record",
          .fail_encryption = {},  // not used
          .fail_decryption = true,
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewClientError(
                  mojom::RegisterClientError::New(
                      mojom::RegisterClientErrorCode::
                          kVerificationTokenDecryptionFailed))),
      });
  return kRegisterPasswordFinalizeVerificationTokenDecryptionFailed.get();
}

const RegisterPasswordFinalizeTestCase*
RegisterPasswordFinalizeBodyMissingOrFailedToParse() {
  static const base::NoDestructor<RegisterPasswordFinalizeTestCase>
      kRegisterPasswordFinalizeBodyMissingOrFailedToParse({
          .test_name =
              "register_password_finalize_body_missing_or_failed_to_parse",
          .encrypted_verification_token = EncryptedVerificationToken(),
          .serialized_record = "serialized_record",
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewServerError(
                  mojom::RegisterServerError::New(
                      net::HTTP_INTERNAL_SERVER_ERROR,
                      mojom::RegisterServerErrorCode::kInvalidResponse))),
      });
  return kRegisterPasswordFinalizeBodyMissingOrFailedToParse.get();
}

const RegisterPasswordFinalizeTestCase*
RegisterPasswordFinalizeErrorCodeIsNull() {
  static const base::NoDestructor<RegisterPasswordFinalizeTestCase>
      kRegisterPasswordFinalizeErrorCodeIsNull({
          .test_name = "register_password_finalize_error_code_is_null",
          .encrypted_verification_token = EncryptedVerificationToken(),
          .serialized_record = "serialized_record",
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_NOT_FOUND,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewServerError(
                  mojom::RegisterServerError::New(
                      net::HTTP_NOT_FOUND,
                      mojom::RegisterServerErrorCode::kNull))),
      });
  return kRegisterPasswordFinalizeErrorCodeIsNull.get();
}

const RegisterPasswordFinalizeTestCase*
RegisterPasswordFinalizeUnknownErrorCode() {
  static const base::NoDestructor<RegisterPasswordFinalizeTestCase>
      kRegisterPasswordFinalizeUnknownErrorCode({
          .test_name = "register_password_finalize_unknown_error_code",
          .encrypted_verification_token = EncryptedVerificationToken(),
          .serialized_record = "serialized_record",
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_TOO_EARLY,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value(42);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewServerError(
                  mojom::RegisterServerError::New(
                      net::HTTP_TOO_EARLY,
                      mojom::RegisterServerErrorCode::kUnknown))),
      });
  return kRegisterPasswordFinalizeUnknownErrorCode.get();
}

const RegisterPasswordFinalizeTestCase*
RegisterPasswordFinalizeKnownErrorCode() {
  static const base::NoDestructor<RegisterPasswordFinalizeTestCase>
      kRegisterPasswordFinalizeKnownErrorCode({
          .test_name = "register_password_finalize_known_error_code",
          .encrypted_verification_token = EncryptedVerificationToken(),
          .serialized_record = "serialized_record",
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_NOT_FOUND,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value(14001);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewServerError(
                  mojom::RegisterServerError::New(
                      net::HTTP_NOT_FOUND, mojom::RegisterServerErrorCode::
                                               kInterimPasswordStateNotFound))),
      });
  return kRegisterPasswordFinalizeKnownErrorCode.get();
}

const RegisterPasswordFinalizeTestCase* RegisterPasswordFinalizeSuccess() {
  static const base::NoDestructor<RegisterPasswordFinalizeTestCase>
      kRegisterPasswordFinalizeSuccess({
          .test_name = "register_password_finalize_success",
          .encrypted_verification_token = EncryptedVerificationToken(),
          .serialized_record = "serialized_record",
          .fail_encryption = {},  // not used
          .fail_decryption = false,
          .endpoint_response =
              {{.net_error = net::OK,
                .status_code = net::HTTP_OK,
                .body = PasswordFinalize::Response::SuccessBody()}},
          .mojo_expected = mojom::RegisterPasswordFinalizeResult::New(),
      });
  return kRegisterPasswordFinalizeSuccess.get();
}

using BraveAccountServiceRegisterPasswordFinalizeTest =
    BraveAccountServiceTest<RegisterPasswordFinalizeTestCase>;

}  // namespace

TEST_P(BraveAccountServiceRegisterPasswordFinalizeTest,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();

  if (const auto& test_case = CHECK_DEREF(this->GetParam());
      test_case.mojo_expected.has_value()) {
    AccountStatePrefs account_state_prefs(pref_service_);
    const auto state = account_state_prefs.GetAccountState();
    ASSERT_TRUE(state->is_logged_out());
    ASSERT_TRUE(state->get_logged_out()->verification);
    EXPECT_EQ(state->get_logged_out()->verification->intent,
              mojom::LoggedOutVerificationIntent::kRegistration);
    EXPECT_EQ(account_state_prefs.GetVerificationToken(
                  mojom::VerificationIntent::NewLoggedOutIntent(
                      mojom::LoggedOutVerificationIntent::kRegistration)),
              test_case.encrypted_verification_token);
  }
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceRegisterPasswordFinalizeTest,
    testing::Values(RegisterPasswordFinalizeVerificationTokenDecryptionFailed(),
                    RegisterPasswordFinalizeBodyMissingOrFailedToParse(),
                    RegisterPasswordFinalizeErrorCodeIsNull(),
                    RegisterPasswordFinalizeUnknownErrorCode(),
                    RegisterPasswordFinalizeKnownErrorCode(),
                    RegisterPasswordFinalizeSuccess()),
    BraveAccountServiceRegisterPasswordFinalizeTest::kNameGenerator);

struct RegisterVerifyCompleteTestCase {
  using Endpoint = endpoints::VerifyComplete;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::RegisterVerifyCompleteResultPtr,
                                      mojom::RegisterErrorPtr>;

  static void Run(const RegisterVerifyCompleteTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    AccountStatePrefs(pref_service)
        .AddVerification(EncryptedVerificationToken(),
                         mojom::VerificationIntent::NewLoggedOutIntent(
                             test_case.logged_out_verification_intent));

    authentication->RegisterVerifyComplete(
        test_case.code,
        std::move(callback).Then(base::BindOnce(
            [](PrefService* pref_service,
               mojom::LoggedOutVerificationIntent
                   logged_out_verification_intent,
               bool success) {
              AccountStatePrefs account_state_prefs(*pref_service);
              const auto state = account_state_prefs.GetAccountState();
              if (success) {
                ASSERT_TRUE(state->is_logged_in());
                EXPECT_EQ(state->get_logged_in()->email, kEmailAddress);
                EXPECT_FALSE(state->get_logged_in()->verification);
                EXPECT_EQ(account_state_prefs.GetAuthenticationToken(),
                          EncryptedAuthenticationToken());
              } else {
                ASSERT_TRUE(state->is_logged_out());
                ASSERT_TRUE(state->get_logged_out()->verification);
                EXPECT_EQ(state->get_logged_out()->verification->intent,
                          logged_out_verification_intent);
                EXPECT_EQ(account_state_prefs.GetVerificationToken(
                              mojom::VerificationIntent::NewLoggedOutIntent(
                                  logged_out_verification_intent)),
                          EncryptedVerificationToken());
              }
            },
            base::Unretained(&pref_service),
            test_case.logged_out_verification_intent,
            test_case.mojo_expected.has_value())));
  }

  std::string test_name;
  std::string code;
  mojom::LoggedOutVerificationIntent logged_out_verification_intent;
  bool fail_decryption;
  bool fail_encryption;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const RegisterVerifyCompleteTestCase*
RegisterVerifyCompleteCalledInWrongState() {
  static const base::NoDestructor<RegisterVerifyCompleteTestCase>
      kRegisterVerifyCompleteCalledInWrongState({
          .test_name = "register_verify_complete_called_in_wrong_state",
          .code = "23TZMP",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kResetPassword,
          .fail_decryption = {},    // not used
          .fail_encryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewClientError(
                  mojom::RegisterClientError::New(
                      mojom::RegisterClientErrorCode::kCalledInWrongState))),
      });
  return kRegisterVerifyCompleteCalledInWrongState.get();
}

const RegisterVerifyCompleteTestCase*
RegisterVerifyCompleteVerificationTokenDecryptionFailed() {
  static const base::NoDestructor<RegisterVerifyCompleteTestCase>
      kRegisterVerifyCompleteVerificationTokenDecryptionFailed({
          .test_name =
              "register_verify_complete_verification_token_decryption_failed",
          .code = "23TZMP",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kRegistration,
          .fail_decryption = true,
          .fail_encryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewClientError(
                  mojom::RegisterClientError::New(
                      mojom::RegisterClientErrorCode::
                          kVerificationTokenDecryptionFailed))),
      });
  return kRegisterVerifyCompleteVerificationTokenDecryptionFailed.get();
}

const RegisterVerifyCompleteTestCase*
RegisterVerifyCompleteBodyMissingOrFailedToParse() {
  static const base::NoDestructor<RegisterVerifyCompleteTestCase>
      kRegisterVerifyCompleteBodyMissingOrFailedToParse({
          .test_name =
              "register_verify_complete_body_missing_or_failed_to_parse",
          .code = "23TZMP",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kRegistration,
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewServerError(
                  mojom::RegisterServerError::New(
                      net::HTTP_INTERNAL_SERVER_ERROR,
                      mojom::RegisterServerErrorCode::kInvalidResponse))),
      });
  return kRegisterVerifyCompleteBodyMissingOrFailedToParse.get();
}

const RegisterVerifyCompleteTestCase* RegisterVerifyCompleteErrorCodeIsNull() {
  static const base::NoDestructor<RegisterVerifyCompleteTestCase>
      kRegisterVerifyCompleteErrorCodeIsNull({
          .test_name = "register_verify_complete_error_code_is_null",
          .code = "23TZMP",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kRegistration,
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   VerifyComplete::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewServerError(
                  mojom::RegisterServerError::New(
                      net::HTTP_UNAUTHORIZED,
                      mojom::RegisterServerErrorCode::kNull))),
      });
  return kRegisterVerifyCompleteErrorCodeIsNull.get();
}

const RegisterVerifyCompleteTestCase* RegisterVerifyCompleteUnknownErrorCode() {
  static const base::NoDestructor<RegisterVerifyCompleteTestCase>
      kRegisterVerifyCompleteUnknownErrorCode({
          .test_name = "register_verify_complete_unknown_error_code",
          .code = "23TZMP",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kRegistration,
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_TOO_EARLY,
                                 .body = base::unexpected([] {
                                   VerifyComplete::Response::ErrorBody body;
                                   body.code = base::Value(42);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewServerError(
                  mojom::RegisterServerError::New(
                      net::HTTP_TOO_EARLY,
                      mojom::RegisterServerErrorCode::kUnknown))),
      });
  return kRegisterVerifyCompleteUnknownErrorCode.get();
}

const RegisterVerifyCompleteTestCase* RegisterVerifyCompleteKnownErrorCode() {
  static const base::NoDestructor<RegisterVerifyCompleteTestCase>
      kRegisterVerifyCompleteKnownErrorCode({
          .test_name = "register_verify_complete_known_error_code",
          .code = "23TZMP",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kRegistration,
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_NOT_FOUND,
                                 .body = base::unexpected([] {
                                   VerifyComplete::Response::ErrorBody body;
                                   body.code = base::Value(13002);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewServerError(
                  mojom::RegisterServerError::New(
                      net::HTTP_NOT_FOUND,
                      mojom::RegisterServerErrorCode::
                          kVerificationNotFoundOrInvalidIdOrCode))),
      });
  return kRegisterVerifyCompleteKnownErrorCode.get();
}

const RegisterVerifyCompleteTestCase* RegisterVerifyCompleteAuthTokenNull() {
  static const base::NoDestructor<RegisterVerifyCompleteTestCase>
      kRegisterVerifyCompleteAuthTokenNull({
          .test_name = "register_verify_complete_auth_token_null",
          .code = "23TZMP",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kRegistration,
          .fail_decryption = false,
          .fail_encryption = {},  // not used
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
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewServerError(
                  mojom::RegisterServerError::New(
                      net::HTTP_OK,
                      mojom::RegisterServerErrorCode::kInvalidResponse))),
      });
  return kRegisterVerifyCompleteAuthTokenNull.get();
}

const RegisterVerifyCompleteTestCase* RegisterVerifyCompleteAuthTokenEmpty() {
  static const base::NoDestructor<RegisterVerifyCompleteTestCase>
      kRegisterVerifyCompleteAuthTokenEmpty({
          .test_name = "register_verify_complete_auth_token_empty",
          .code = "23TZMP",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kRegistration,
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       VerifyComplete::Response::SuccessBody
                                           body;
                                       body.auth_token = base::Value("");
                                       body.email = kEmailAddress;
                                       return body;
                                     }()}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewServerError(
                  mojom::RegisterServerError::New(
                      net::HTTP_OK,
                      mojom::RegisterServerErrorCode::kInvalidResponse))),
      });
  return kRegisterVerifyCompleteAuthTokenEmpty.get();
}

const RegisterVerifyCompleteTestCase* RegisterVerifyCompleteEmailEmpty() {
  static const base::NoDestructor<RegisterVerifyCompleteTestCase>
      kRegisterVerifyCompleteEmailEmpty({
          .test_name = "register_verify_complete_email_empty",
          .code = "23TZMP",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kRegistration,
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response =
              {{.net_error = net::OK,
                .status_code = net::HTTP_OK,
                .body =
                    [] {
                      VerifyComplete::Response::SuccessBody body;
                      body.auth_token = base::Value(kAuthenticationToken);
                      body.email = "";
                      return body;
                    }()}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewServerError(
                  mojom::RegisterServerError::New(
                      net::HTTP_OK,
                      mojom::RegisterServerErrorCode::kInvalidResponse))),
      });
  return kRegisterVerifyCompleteEmailEmpty.get();
}

const RegisterVerifyCompleteTestCase*
RegisterVerifyCompleteAuthenticationTokenEncryptionFailed() {
  static const base::NoDestructor<RegisterVerifyCompleteTestCase>
      kRegisterVerifyCompleteAuthenticationTokenEncryptionFailed({
          .test_name =
              "register_verify_complete_authentication_token_encryption_failed",
          .code = "23TZMP",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kRegistration,
          .fail_decryption = false,
          .fail_encryption = true,
          .endpoint_response =
              {{.net_error = net::OK,
                .status_code = net::HTTP_OK,
                .body =
                    [] {
                      VerifyComplete::Response::SuccessBody body;
                      body.auth_token = base::Value(kAuthenticationToken);
                      body.email = kEmailAddress;
                      return body;
                    }()}},
          .mojo_expected =
              base::unexpected(mojom::RegisterError::NewClientError(
                  mojom::RegisterClientError::New(
                      mojom::RegisterClientErrorCode::
                          kAuthenticationTokenEncryptionFailed))),
      });
  return kRegisterVerifyCompleteAuthenticationTokenEncryptionFailed.get();
}

const RegisterVerifyCompleteTestCase* RegisterVerifyCompleteSuccess() {
  static const base::NoDestructor<RegisterVerifyCompleteTestCase>
      kRegisterVerifyCompleteSuccess({
          .test_name = "register_verify_complete_success",
          .code = "23TZMP",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kRegistration,
          .fail_decryption = false,
          .fail_encryption = false,
          .endpoint_response =
              {{.net_error = net::OK,
                .status_code = net::HTTP_OK,
                .body =
                    [] {
                      VerifyComplete::Response::SuccessBody body;
                      body.auth_token = base::Value(kAuthenticationToken);
                      body.email = kEmailAddress;
                      return body;
                    }()}},
          .mojo_expected = mojom::RegisterVerifyCompleteResult::New(),
      });
  return kRegisterVerifyCompleteSuccess.get();
}

using BraveAccountServiceRegisterVerifyCompleteTest =
    BraveAccountServiceTest<RegisterVerifyCompleteTestCase>;

}  // namespace

TEST_P(BraveAccountServiceRegisterVerifyCompleteTest,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceRegisterVerifyCompleteTest,
    testing::Values(RegisterVerifyCompleteCalledInWrongState(),
                    RegisterVerifyCompleteVerificationTokenDecryptionFailed(),
                    RegisterVerifyCompleteBodyMissingOrFailedToParse(),
                    RegisterVerifyCompleteErrorCodeIsNull(),
                    RegisterVerifyCompleteUnknownErrorCode(),
                    RegisterVerifyCompleteKnownErrorCode(),
                    RegisterVerifyCompleteAuthTokenNull(),
                    RegisterVerifyCompleteAuthTokenEmpty(),
                    RegisterVerifyCompleteEmailEmpty(),
                    RegisterVerifyCompleteAuthenticationTokenEncryptionFailed(),
                    RegisterVerifyCompleteSuccess()),
    BraveAccountServiceRegisterVerifyCompleteTest::kNameGenerator);

}  // namespace brave_account
