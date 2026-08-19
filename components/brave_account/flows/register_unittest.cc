/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/register.h"

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
#include "brave/components/brave_account/mojom/register.mojom.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

using endpoints::PasswordFinalize;
using endpoints::PasswordInit;
using endpoints::VerifyComplete;

struct RegisterStep1TestCase {
  using Endpoint = PasswordInit;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected =
      base::expected<mojom::RegisterStep1ResultPtr, mojom::RegisterErrorPtr>;

  static void Run(const RegisterStep1TestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    authentication->RegisterStep1(mojom::Service::kAccounts, test_case.email,
                                  test_case.blinded_message,
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

const RegisterStep1TestCase* RegisterStep1BodyMissingOrFailedToParse() {
  static const base::NoDestructor<RegisterStep1TestCase> kTestCase({
      .test_name = "register_step1_body_missing_or_failed_to_parse",
      .email = kEmailAddress,
      .blinded_message = "blinded_message",
      .fail_encryption = {},  // not used
      .fail_decryption = {},  // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                             .body = std::nullopt}},
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewServerError(mojom::RegisterServerError::New(
              net::HTTP_INTERNAL_SERVER_ERROR,
              mojom::RegisterServerErrorCode::kInvalidResponse))),
  });
  return kTestCase.get();
}

const RegisterStep1TestCase* RegisterStep1ErrorCodeIsNull() {
  static const base::NoDestructor<RegisterStep1TestCase> kTestCase({
      .test_name = "register_step1_error_code_is_null",
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
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewServerError(mojom::RegisterServerError::New(
              net::HTTP_BAD_REQUEST, mojom::RegisterServerErrorCode::kNull))),
  });
  return kTestCase.get();
}

const RegisterStep1TestCase* RegisterStep1UnknownErrorCode() {
  static const base::NoDestructor<RegisterStep1TestCase> kTestCase({
      .test_name = "register_step1_unknown_error_code",
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
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewServerError(mojom::RegisterServerError::New(
              net::HTTP_TOO_EARLY, mojom::RegisterServerErrorCode::kUnknown))),
  });
  return kTestCase.get();
}

const RegisterStep1TestCase* RegisterStep1KnownErrorCode() {
  static const base::NoDestructor<RegisterStep1TestCase> kTestCase({
      .test_name = "register_step1_known_error_code",
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
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewServerError(mojom::RegisterServerError::New(
              net::HTTP_BAD_REQUEST,
              mojom::RegisterServerErrorCode::kNewAccountEmailRequired))),
  });
  return kTestCase.get();
}

const RegisterStep1TestCase* RegisterStep1VerificationTokenMissing() {
  static const base::NoDestructor<RegisterStep1TestCase> kTestCase({
      .test_name = "register_step1_verification_token_missing",
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
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewServerError(mojom::RegisterServerError::New(
              net::HTTP_OK, mojom::RegisterServerErrorCode::kInvalidResponse))),
  });
  return kTestCase.get();
}

const RegisterStep1TestCase* RegisterStep1VerificationTokenEmpty() {
  static const base::NoDestructor<RegisterStep1TestCase> kTestCase({
      .test_name = "register_step1_verification_token_empty",
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
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewServerError(mojom::RegisterServerError::New(
              net::HTTP_OK, mojom::RegisterServerErrorCode::kInvalidResponse))),
  });
  return kTestCase.get();
}

const RegisterStep1TestCase* RegisterStep1SerializedResponseEmpty() {
  static const base::NoDestructor<RegisterStep1TestCase> kTestCase({
      .test_name = "register_step1_serialized_response_empty",
      .email = kEmailAddress,
      .blinded_message = "blinded_message",
      .fail_encryption = {},  // not used
      .fail_decryption = {},  // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   PasswordInit::Response::SuccessBody body;
                                   body.verification_token = kVerificationToken;
                                   body.serialized_response = "";
                                   return body;
                                 }()}},
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewServerError(mojom::RegisterServerError::New(
              net::HTTP_OK, mojom::RegisterServerErrorCode::kInvalidResponse))),
  });
  return kTestCase.get();
}

const RegisterStep1TestCase* RegisterStep1VerificationTokenEncryptionFailed() {
  static const base::NoDestructor<RegisterStep1TestCase> kTestCase({
      .test_name = "register_step1_verification_token_encryption_failed",
      .email = kEmailAddress,
      .blinded_message = "blinded_message",
      .fail_encryption = true,
      .fail_decryption = {},  // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   PasswordInit::Response::SuccessBody body;
                                   body.verification_token = kVerificationToken;
                                   body.serialized_response =
                                       "serialized_response";
                                   return body;
                                 }()}},
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewClientError(mojom::RegisterClientError::New(
              mojom::RegisterClientErrorCode::
                  kVerificationTokenEncryptionFailed))),
  });
  return kTestCase.get();
}

const RegisterStep1TestCase* RegisterStep1Success() {
  static const base::NoDestructor<RegisterStep1TestCase> kTestCase({
      .test_name = "register_step1_success",
      .email = kEmailAddress,
      .blinded_message = "blinded_message",
      .fail_encryption = false,
      .fail_decryption = {},  // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   PasswordInit::Response::SuccessBody body;
                                   body.verification_token = kVerificationToken;
                                   body.serialized_response =
                                       "serialized_response";
                                   return body;
                                 }()}},
      .mojo_expected = mojom::RegisterStep1Result::New(
          EncryptedVerificationToken(), "serialized_response"),
  });
  return kTestCase.get();
}

using BraveAccountServiceRegisterStep1Test =
    BraveAccountServiceTest<RegisterStep1TestCase>;

}  // namespace

TEST_P(BraveAccountServiceRegisterStep1Test,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceRegisterStep1Test,
    testing::Values(RegisterStep1BodyMissingOrFailedToParse(),
                    RegisterStep1ErrorCodeIsNull(),
                    RegisterStep1UnknownErrorCode(),
                    RegisterStep1KnownErrorCode(),
                    RegisterStep1VerificationTokenMissing(),
                    RegisterStep1VerificationTokenEmpty(),
                    RegisterStep1SerializedResponseEmpty(),
                    RegisterStep1VerificationTokenEncryptionFailed(),
                    RegisterStep1Success()),
    BraveAccountServiceRegisterStep1Test::kNameGenerator);

struct RegisterStep2TestCase {
  using Endpoint = PasswordFinalize;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected =
      base::expected<mojom::RegisterStep2ResultPtr, mojom::RegisterErrorPtr>;

  static void Run(const RegisterStep2TestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    authentication->RegisterStep2(test_case.encrypted_verification_token,
                                  test_case.serialized_record,
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

const RegisterStep2TestCase* RegisterStep2VerificationTokenDecryptionFailed() {
  static const base::NoDestructor<RegisterStep2TestCase> kTestCase({
      .test_name = "register_step2_verification_token_decryption_failed",
      .encrypted_verification_token = EncryptedVerificationToken(),
      .serialized_record = "serialized_record",
      .fail_encryption = {},  // not used
      .fail_decryption = true,
      .endpoint_response = {},  // not used
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewClientError(mojom::RegisterClientError::New(
              mojom::RegisterClientErrorCode::
                  kVerificationTokenDecryptionFailed))),
  });
  return kTestCase.get();
}

const RegisterStep2TestCase* RegisterStep2BodyMissingOrFailedToParse() {
  static const base::NoDestructor<RegisterStep2TestCase> kTestCase({
      .test_name = "register_step2_body_missing_or_failed_to_parse",
      .encrypted_verification_token = EncryptedVerificationToken(),
      .serialized_record = "serialized_record",
      .fail_encryption = {},  // not used
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                             .body = std::nullopt}},
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewServerError(mojom::RegisterServerError::New(
              net::HTTP_INTERNAL_SERVER_ERROR,
              mojom::RegisterServerErrorCode::kInvalidResponse))),
  });
  return kTestCase.get();
}

const RegisterStep2TestCase* RegisterStep2ErrorCodeIsNull() {
  static const base::NoDestructor<RegisterStep2TestCase> kTestCase({
      .test_name = "register_step2_error_code_is_null",
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
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewServerError(mojom::RegisterServerError::New(
              net::HTTP_NOT_FOUND, mojom::RegisterServerErrorCode::kNull))),
  });
  return kTestCase.get();
}

const RegisterStep2TestCase* RegisterStep2UnknownErrorCode() {
  static const base::NoDestructor<RegisterStep2TestCase> kTestCase({
      .test_name = "register_step2_unknown_error_code",
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
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewServerError(mojom::RegisterServerError::New(
              net::HTTP_TOO_EARLY, mojom::RegisterServerErrorCode::kUnknown))),
  });
  return kTestCase.get();
}

const RegisterStep2TestCase* RegisterStep2KnownErrorCode() {
  static const base::NoDestructor<RegisterStep2TestCase> kTestCase({
      .test_name = "register_step2_known_error_code",
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
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewServerError(mojom::RegisterServerError::New(
              net::HTTP_NOT_FOUND,
              mojom::RegisterServerErrorCode::kInterimPasswordStateNotFound))),
  });
  return kTestCase.get();
}

const RegisterStep2TestCase* RegisterStep2Success() {
  static const base::NoDestructor<RegisterStep2TestCase> kTestCase({
      .test_name = "register_step2_success",
      .encrypted_verification_token = EncryptedVerificationToken(),
      .serialized_record = "serialized_record",
      .fail_encryption = {},  // not used
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 PasswordFinalize::Response::SuccessBody()}},
      .mojo_expected = mojom::RegisterStep2Result::New(),
  });
  return kTestCase.get();
}

using BraveAccountServiceRegisterStep2Test =
    BraveAccountServiceTest<RegisterStep2TestCase>;

}  // namespace

TEST_P(BraveAccountServiceRegisterStep2Test,
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
    BraveAccountServiceRegisterStep2Test,
    testing::Values(RegisterStep2VerificationTokenDecryptionFailed(),
                    RegisterStep2BodyMissingOrFailedToParse(),
                    RegisterStep2ErrorCodeIsNull(),
                    RegisterStep2UnknownErrorCode(),
                    RegisterStep2KnownErrorCode(),
                    RegisterStep2Success()),
    BraveAccountServiceRegisterStep2Test::kNameGenerator);

struct RegisterStep3TestCase {
  using Endpoint = endpoints::VerifyComplete;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected =
      base::expected<mojom::RegisterStep3ResultPtr, mojom::RegisterErrorPtr>;

  static void Run(const RegisterStep3TestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    AccountStatePrefs(pref_service)
        .AddVerification(EncryptedVerificationToken(),
                         mojom::VerificationIntent::NewLoggedOutIntent(
                             test_case.logged_out_verification_intent));

    authentication->RegisterStep3(
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

const RegisterStep3TestCase* RegisterStep3CalledInWrongState() {
  static const base::NoDestructor<RegisterStep3TestCase> kTestCase({
      .test_name = "register_step3_called_in_wrong_state",
      .code = "23TZMP",
      .logged_out_verification_intent =
          mojom::LoggedOutVerificationIntent::kResetPassword,
      .fail_decryption = {},    // not used
      .fail_encryption = {},    // not used
      .endpoint_response = {},  // not used
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewClientError(mojom::RegisterClientError::New(
              mojom::RegisterClientErrorCode::kCalledInWrongState))),
  });
  return kTestCase.get();
}

const RegisterStep3TestCase* RegisterStep3VerificationTokenDecryptionFailed() {
  static const base::NoDestructor<RegisterStep3TestCase> kTestCase({
      .test_name = "register_step3_verification_token_decryption_failed",
      .code = "23TZMP",
      .logged_out_verification_intent =
          mojom::LoggedOutVerificationIntent::kRegistration,
      .fail_decryption = true,
      .fail_encryption = {},    // not used
      .endpoint_response = {},  // not used
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewClientError(mojom::RegisterClientError::New(
              mojom::RegisterClientErrorCode::
                  kVerificationTokenDecryptionFailed))),
  });
  return kTestCase.get();
}

const RegisterStep3TestCase* RegisterStep3BodyMissingOrFailedToParse() {
  static const base::NoDestructor<RegisterStep3TestCase> kTestCase({
      .test_name = "register_step3_body_missing_or_failed_to_parse",
      .code = "23TZMP",
      .logged_out_verification_intent =
          mojom::LoggedOutVerificationIntent::kRegistration,
      .fail_decryption = false,
      .fail_encryption = {},  // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                             .body = std::nullopt}},
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewServerError(mojom::RegisterServerError::New(
              net::HTTP_INTERNAL_SERVER_ERROR,
              mojom::RegisterServerErrorCode::kInvalidResponse))),
  });
  return kTestCase.get();
}

const RegisterStep3TestCase* RegisterStep3ErrorCodeIsNull() {
  static const base::NoDestructor<RegisterStep3TestCase> kTestCase({
      .test_name = "register_step3_error_code_is_null",
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
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewServerError(mojom::RegisterServerError::New(
              net::HTTP_UNAUTHORIZED, mojom::RegisterServerErrorCode::kNull))),
  });
  return kTestCase.get();
}

const RegisterStep3TestCase* RegisterStep3UnknownErrorCode() {
  static const base::NoDestructor<RegisterStep3TestCase> kTestCase({
      .test_name = "register_step3_unknown_error_code",
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
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewServerError(mojom::RegisterServerError::New(
              net::HTTP_TOO_EARLY, mojom::RegisterServerErrorCode::kUnknown))),
  });
  return kTestCase.get();
}

const RegisterStep3TestCase* RegisterStep3KnownErrorCode() {
  static const base::NoDestructor<RegisterStep3TestCase> kTestCase({
      .test_name = "register_step3_known_error_code",
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
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewServerError(mojom::RegisterServerError::New(
              net::HTTP_NOT_FOUND,
              mojom::RegisterServerErrorCode::
                  kVerificationNotFoundOrInvalidIdOrCode))),
  });
  return kTestCase.get();
}

const RegisterStep3TestCase* RegisterStep3AuthTokenNull() {
  static const base::NoDestructor<RegisterStep3TestCase> kTestCase({
      .test_name = "register_step3_auth_token_null",
      .code = "23TZMP",
      .logged_out_verification_intent =
          mojom::LoggedOutVerificationIntent::kRegistration,
      .fail_decryption = false,
      .fail_encryption = {},  // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   VerifyComplete::Response::SuccessBody body;
                                   body.auth_token = base::Value();
                                   body.email = kEmailAddress;
                                   return body;
                                 }()}},
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewServerError(mojom::RegisterServerError::New(
              net::HTTP_OK, mojom::RegisterServerErrorCode::kInvalidResponse))),
  });
  return kTestCase.get();
}

const RegisterStep3TestCase* RegisterStep3AuthTokenEmpty() {
  static const base::NoDestructor<RegisterStep3TestCase> kTestCase({
      .test_name = "register_step3_auth_token_empty",
      .code = "23TZMP",
      .logged_out_verification_intent =
          mojom::LoggedOutVerificationIntent::kRegistration,
      .fail_decryption = false,
      .fail_encryption = {},  // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   VerifyComplete::Response::SuccessBody body;
                                   body.auth_token = base::Value("");
                                   body.email = kEmailAddress;
                                   return body;
                                 }()}},
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewServerError(mojom::RegisterServerError::New(
              net::HTTP_OK, mojom::RegisterServerErrorCode::kInvalidResponse))),
  });
  return kTestCase.get();
}

const RegisterStep3TestCase* RegisterStep3EmailEmpty() {
  static const base::NoDestructor<RegisterStep3TestCase> kTestCase({
      .test_name = "register_step3_email_empty",
      .code = "23TZMP",
      .logged_out_verification_intent =
          mojom::LoggedOutVerificationIntent::kRegistration,
      .fail_decryption = false,
      .fail_encryption = {},  // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   VerifyComplete::Response::SuccessBody body;
                                   body.auth_token =
                                       base::Value(kAuthenticationToken);
                                   body.email = "";
                                   return body;
                                 }()}},
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewServerError(mojom::RegisterServerError::New(
              net::HTTP_OK, mojom::RegisterServerErrorCode::kInvalidResponse))),
  });
  return kTestCase.get();
}

const RegisterStep3TestCase*
RegisterStep3AuthenticationTokenEncryptionFailed() {
  static const base::NoDestructor<RegisterStep3TestCase> kTestCase({
      .test_name = "register_step3_authentication_token_encryption_failed",
      .code = "23TZMP",
      .logged_out_verification_intent =
          mojom::LoggedOutVerificationIntent::kRegistration,
      .fail_decryption = false,
      .fail_encryption = true,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   VerifyComplete::Response::SuccessBody body;
                                   body.auth_token =
                                       base::Value(kAuthenticationToken);
                                   body.email = kEmailAddress;
                                   return body;
                                 }()}},
      .mojo_expected = base::unexpected(
          mojom::RegisterError::NewClientError(mojom::RegisterClientError::New(
              mojom::RegisterClientErrorCode::
                  kAuthenticationTokenEncryptionFailed))),
  });
  return kTestCase.get();
}

const RegisterStep3TestCase* RegisterStep3Success() {
  static const base::NoDestructor<RegisterStep3TestCase> kTestCase({
      .test_name = "register_step3_success",
      .code = "23TZMP",
      .logged_out_verification_intent =
          mojom::LoggedOutVerificationIntent::kRegistration,
      .fail_decryption = false,
      .fail_encryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   VerifyComplete::Response::SuccessBody body;
                                   body.auth_token =
                                       base::Value(kAuthenticationToken);
                                   body.email = kEmailAddress;
                                   return body;
                                 }()}},
      .mojo_expected = mojom::RegisterStep3Result::New(),
  });
  return kTestCase.get();
}

using BraveAccountServiceRegisterStep3Test =
    BraveAccountServiceTest<RegisterStep3TestCase>;

}  // namespace

TEST_P(BraveAccountServiceRegisterStep3Test,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceRegisterStep3Test,
    testing::Values(RegisterStep3CalledInWrongState(),
                    RegisterStep3VerificationTokenDecryptionFailed(),
                    RegisterStep3BodyMissingOrFailedToParse(),
                    RegisterStep3ErrorCodeIsNull(),
                    RegisterStep3UnknownErrorCode(),
                    RegisterStep3KnownErrorCode(),
                    RegisterStep3AuthTokenNull(),
                    RegisterStep3AuthTokenEmpty(),
                    RegisterStep3EmailEmpty(),
                    RegisterStep3AuthenticationTokenEncryptionFailed(),
                    RegisterStep3Success()),
    BraveAccountServiceRegisterStep3Test::kNameGenerator);

}  // namespace brave_account
