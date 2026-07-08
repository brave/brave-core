/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/change_password.h"

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
#include "brave/components/brave_account/mojom/change_password.mojom.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

using endpoints::PasswordFinalize;
using endpoints::PasswordInit;
using endpoints::VerifyComplete;
using endpoints::VerifyInit;

// All `ChangePassword*` methods require the user to be logged in, and the steps
// after `ChangePasswordVerifyInit` additionally require the verification slot
// it would have produced. `SeedLoggedIn()`'s `seed_verification` flag selects
// whether to seed that slot before invoking the method under test.
namespace {

void SeedLoggedIn(PrefService& pref_service, bool seed_verification = false) {
  AccountStatePrefs account_state_prefs(pref_service);
  account_state_prefs.SetLoggedIn(kEmailAddress,
                                  EncryptedAuthenticationToken());

  if (seed_verification) {
    account_state_prefs.AddVerification(
        EncryptedVerificationToken(),
        mojom::VerificationIntent::NewLoggedInIntent(
            mojom::LoggedInVerificationIntent::kChangePassword));
  }
}

}  // namespace

struct ChangePasswordVerifyInitTestCase {
  using Endpoint = VerifyInit;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::ChangePasswordVerifyInitResultPtr,
                                      mojom::ChangePasswordErrorPtr>;

  static void Run(const ChangePasswordVerifyInitTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    SeedLoggedIn(pref_service);

    authentication->ChangePasswordVerifyInit(
        kEmailAddress,
        std::move(callback).Then(base::BindOnce(
            [](PrefService* pref_service, bool success) {
              AccountStatePrefs account_state_prefs(*pref_service);
              const auto state = account_state_prefs.GetAccountState();
              ASSERT_TRUE(state->is_logged_in());
              EXPECT_EQ(state->get_logged_in()->email, kEmailAddress);
              EXPECT_EQ(account_state_prefs.GetAuthenticationToken(),
                        EncryptedAuthenticationToken());
              if (success) {
                ASSERT_TRUE(state->get_logged_in()->verification);
                EXPECT_EQ(state->get_logged_in()->verification->intent,
                          mojom::LoggedInVerificationIntent::kChangePassword);
                EXPECT_EQ(account_state_prefs.GetVerificationToken(
                              mojom::VerificationIntent::NewLoggedInIntent(
                                  mojom::LoggedInVerificationIntent::
                                      kChangePassword)),
                          EncryptedVerificationToken());
              } else {
                EXPECT_FALSE(state->get_logged_in()->verification);
              }
            },
            base::Unretained(&pref_service),
            test_case.mojo_expected.has_value())));
  }

  std::string test_name;
  bool fail_decryption;
  bool fail_encryption;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const ChangePasswordVerifyInitTestCase*
ChangePasswordVerifyInitAuthenticationTokenDecryptionFailed() {
  static const base::NoDestructor<ChangePasswordVerifyInitTestCase>
      kChangePasswordVerifyInitAuthenticationTokenDecryptionFailed({
          .test_name = "change_password_verify_init_authentication_token_"
                       "decryption_failed",
          .fail_decryption = true,
          .fail_encryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::ChangePasswordError::NewClientError(
                  mojom::ChangePasswordClientError::New(
                      mojom::ChangePasswordClientErrorCode::
                          kAuthenticationTokenDecryptionFailed))),
      });
  return kChangePasswordVerifyInitAuthenticationTokenDecryptionFailed.get();
}

const ChangePasswordVerifyInitTestCase*
ChangePasswordVerifyInitBodyMissingOrFailedToParse() {
  static const base::NoDestructor<ChangePasswordVerifyInitTestCase>
      kChangePasswordVerifyInitBodyMissingOrFailedToParse({
          .test_name =
              "change_password_verify_init_body_missing_or_failed_to_parse",
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .mojo_expected =
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_INTERNAL_SERVER_ERROR,
                      mojom::ChangePasswordServerErrorCode::kInvalidResponse))),
      });
  return kChangePasswordVerifyInitBodyMissingOrFailedToParse.get();
}

const ChangePasswordVerifyInitTestCase*
ChangePasswordVerifyInitErrorCodeIsNull() {
  static const base::NoDestructor<ChangePasswordVerifyInitTestCase>
      kChangePasswordVerifyInitErrorCodeIsNull({
          .test_name = "change_password_verify_init_error_code_is_null",
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   VerifyInit::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::ChangePasswordServerErrorCode::kNull))),
      });
  return kChangePasswordVerifyInitErrorCodeIsNull.get();
}

const ChangePasswordVerifyInitTestCase*
ChangePasswordVerifyInitUnknownErrorCode() {
  static const base::NoDestructor<ChangePasswordVerifyInitTestCase>
      kChangePasswordVerifyInitUnknownErrorCode({
          .test_name = "change_password_verify_init_unknown_error_code",
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_TOO_EARLY,
                                 .body = base::unexpected([] {
                                   VerifyInit::Response::ErrorBody body;
                                   body.code = base::Value(42);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_TOO_EARLY,
                      mojom::ChangePasswordServerErrorCode::kUnknown))),
      });
  return kChangePasswordVerifyInitUnknownErrorCode.get();
}

const ChangePasswordVerifyInitTestCase*
ChangePasswordVerifyInitKnownErrorCode() {
  static const base::NoDestructor<ChangePasswordVerifyInitTestCase>
      kChangePasswordVerifyInitKnownErrorCode({
          .test_name = "change_password_verify_init_known_error_code",
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   VerifyInit::Response::ErrorBody body;
                                   body.code = base::Value(13001);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::ChangePasswordServerErrorCode::
                          kTooManyVerifications))),
      });
  return kChangePasswordVerifyInitKnownErrorCode.get();
}

const ChangePasswordVerifyInitTestCase*
ChangePasswordVerifyInitVerificationTokenEmpty() {
  static const base::NoDestructor<ChangePasswordVerifyInitTestCase>
      kChangePasswordVerifyInitVerificationTokenEmpty({
          .test_name = "change_password_verify_init_verification_token_empty",
          .fail_decryption = false,
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
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_OK,
                      mojom::ChangePasswordServerErrorCode::kInvalidResponse))),
      });
  return kChangePasswordVerifyInitVerificationTokenEmpty.get();
}

const ChangePasswordVerifyInitTestCase*
ChangePasswordVerifyInitVerificationTokenEncryptionFailed() {
  static const base::NoDestructor<ChangePasswordVerifyInitTestCase>
      kChangePasswordVerifyInitVerificationTokenEncryptionFailed({
          .test_name = "change_password_verify_init_verification_token_"
                       "encryption_failed",
          .fail_decryption = false,
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
              base::unexpected(mojom::ChangePasswordError::NewClientError(
                  mojom::ChangePasswordClientError::New(
                      mojom::ChangePasswordClientErrorCode::
                          kVerificationTokenEncryptionFailed))),
      });
  return kChangePasswordVerifyInitVerificationTokenEncryptionFailed.get();
}

const ChangePasswordVerifyInitTestCase* ChangePasswordVerifyInitSuccess() {
  static const base::NoDestructor<ChangePasswordVerifyInitTestCase>
      kChangePasswordVerifyInitSuccess({
          .test_name = "change_password_verify_init_success",
          .fail_decryption = false,
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
          .mojo_expected = mojom::ChangePasswordVerifyInitResult::New(),
      });
  return kChangePasswordVerifyInitSuccess.get();
}

using BraveAccountServiceChangePasswordVerifyInitTest =
    BraveAccountServiceTest<ChangePasswordVerifyInitTestCase>;

}  // namespace

TEST_P(BraveAccountServiceChangePasswordVerifyInitTest,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceChangePasswordVerifyInitTest,
    testing::Values(
        ChangePasswordVerifyInitAuthenticationTokenDecryptionFailed(),
        ChangePasswordVerifyInitBodyMissingOrFailedToParse(),
        ChangePasswordVerifyInitErrorCodeIsNull(),
        ChangePasswordVerifyInitUnknownErrorCode(),
        ChangePasswordVerifyInitKnownErrorCode(),
        ChangePasswordVerifyInitVerificationTokenEmpty(),
        ChangePasswordVerifyInitVerificationTokenEncryptionFailed(),
        ChangePasswordVerifyInitSuccess()),
    BraveAccountServiceChangePasswordVerifyInitTest::kNameGenerator);

struct ChangePasswordVerifyCompleteTestCase {
  using Endpoint = VerifyComplete;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected =
      base::expected<mojom::ChangePasswordVerifyCompleteResultPtr,
                     mojom::ChangePasswordErrorPtr>;

  static void Run(const ChangePasswordVerifyCompleteTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    SeedLoggedIn(pref_service, test_case.seed_verification);

    authentication->ChangePasswordVerifyComplete(
        "23TZMP",
        std::move(callback).Then(base::BindOnce(
            [](PrefService* pref_service, bool success) {
              if (!success) {
                return;
              }

              AccountStatePrefs account_state_prefs(*pref_service);
              const auto state = account_state_prefs.GetAccountState();
              ASSERT_TRUE(state->is_logged_in());
              ASSERT_TRUE(state->get_logged_in()->verification);
              EXPECT_EQ(state->get_logged_in()->verification->intent,
                        mojom::LoggedInVerificationIntent::kChangePassword);
              EXPECT_EQ(state->get_logged_in()->verification->verified_email,
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

const ChangePasswordVerifyCompleteTestCase*
ChangePasswordVerifyCompleteCalledInWrongState() {
  static const base::NoDestructor<ChangePasswordVerifyCompleteTestCase>
      kChangePasswordVerifyCompleteCalledInWrongState({
          .test_name = "change_password_verify_complete_called_in_wrong_state",
          .seed_verification = false,
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::ChangePasswordError::NewClientError(
                  mojom::ChangePasswordClientError::New(
                      mojom::ChangePasswordClientErrorCode::
                          kCalledInWrongState))),
      });
  return kChangePasswordVerifyCompleteCalledInWrongState.get();
}

const ChangePasswordVerifyCompleteTestCase*
ChangePasswordVerifyCompleteVerificationTokenDecryptionFailed() {
  static const base::NoDestructor<ChangePasswordVerifyCompleteTestCase>
      kChangePasswordVerifyCompleteVerificationTokenDecryptionFailed({
          .test_name = "change_password_verify_complete_verification_token_"
                       "decryption_failed",
          .seed_verification = true,
          .fail_decryption = true,
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::ChangePasswordError::NewClientError(
                  mojom::ChangePasswordClientError::New(
                      mojom::ChangePasswordClientErrorCode::
                          kVerificationTokenDecryptionFailed))),
      });
  return kChangePasswordVerifyCompleteVerificationTokenDecryptionFailed.get();
}

const ChangePasswordVerifyCompleteTestCase*
ChangePasswordVerifyCompleteBodyMissingOrFailedToParse() {
  static const base::NoDestructor<ChangePasswordVerifyCompleteTestCase>
      kChangePasswordVerifyCompleteBodyMissingOrFailedToParse({
          .test_name =
              "change_password_verify_complete_body_missing_or_failed_to_parse",
          .seed_verification = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .mojo_expected =
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_INTERNAL_SERVER_ERROR,
                      mojom::ChangePasswordServerErrorCode::kInvalidResponse))),
      });
  return kChangePasswordVerifyCompleteBodyMissingOrFailedToParse.get();
}

const ChangePasswordVerifyCompleteTestCase*
ChangePasswordVerifyCompleteErrorCodeIsNull() {
  static const base::NoDestructor<ChangePasswordVerifyCompleteTestCase>
      kChangePasswordVerifyCompleteErrorCodeIsNull({
          .test_name = "change_password_verify_complete_error_code_is_null",
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
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_UNAUTHORIZED,
                      mojom::ChangePasswordServerErrorCode::kNull))),
      });
  return kChangePasswordVerifyCompleteErrorCodeIsNull.get();
}

const ChangePasswordVerifyCompleteTestCase*
ChangePasswordVerifyCompleteUnknownErrorCode() {
  static const base::NoDestructor<ChangePasswordVerifyCompleteTestCase>
      kChangePasswordVerifyCompleteUnknownErrorCode({
          .test_name = "change_password_verify_complete_unknown_error_code",
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
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_TOO_EARLY,
                      mojom::ChangePasswordServerErrorCode::kUnknown))),
      });
  return kChangePasswordVerifyCompleteUnknownErrorCode.get();
}

const ChangePasswordVerifyCompleteTestCase*
ChangePasswordVerifyCompleteKnownErrorCode() {
  static const base::NoDestructor<ChangePasswordVerifyCompleteTestCase>
      kChangePasswordVerifyCompleteKnownErrorCode({
          .test_name = "change_password_verify_complete_known_error_code",
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
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::ChangePasswordServerErrorCode::
                          kInvalidVerificationCode))),
      });
  return kChangePasswordVerifyCompleteKnownErrorCode.get();
}

const ChangePasswordVerifyCompleteTestCase*
ChangePasswordVerifyCompleteEmailEmpty() {
  static const base::NoDestructor<ChangePasswordVerifyCompleteTestCase>
      kChangePasswordVerifyCompleteEmailEmpty({
          .test_name = "change_password_verify_complete_email_empty",
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
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_OK,
                      mojom::ChangePasswordServerErrorCode::kInvalidResponse))),
      });
  return kChangePasswordVerifyCompleteEmailEmpty.get();
}

const ChangePasswordVerifyCompleteTestCase*
ChangePasswordVerifyCompleteSuccess() {
  static const base::NoDestructor<ChangePasswordVerifyCompleteTestCase>
      kChangePasswordVerifyCompleteSuccess({
          .test_name = "change_password_verify_complete_success",
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
          .mojo_expected = mojom::ChangePasswordVerifyCompleteResult::New(),
      });
  return kChangePasswordVerifyCompleteSuccess.get();
}

using BraveAccountServiceChangePasswordVerifyCompleteTest =
    BraveAccountServiceTest<ChangePasswordVerifyCompleteTestCase>;

}  // namespace

TEST_P(BraveAccountServiceChangePasswordVerifyCompleteTest,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceChangePasswordVerifyCompleteTest,
    testing::Values(
        ChangePasswordVerifyCompleteCalledInWrongState(),
        ChangePasswordVerifyCompleteVerificationTokenDecryptionFailed(),
        ChangePasswordVerifyCompleteBodyMissingOrFailedToParse(),
        ChangePasswordVerifyCompleteErrorCodeIsNull(),
        ChangePasswordVerifyCompleteUnknownErrorCode(),
        ChangePasswordVerifyCompleteKnownErrorCode(),
        ChangePasswordVerifyCompleteEmailEmpty(),
        ChangePasswordVerifyCompleteSuccess()),
    BraveAccountServiceChangePasswordVerifyCompleteTest::kNameGenerator);

struct ChangePasswordPasswordInitTestCase {
  using Endpoint = PasswordInit;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected =
      base::expected<mojom::ChangePasswordPasswordInitResultPtr,
                     mojom::ChangePasswordErrorPtr>;

  static void Run(const ChangePasswordPasswordInitTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    SeedLoggedIn(pref_service, test_case.seed_verification);

    authentication->ChangePasswordPasswordInit("blinded_message",
                                               std::move(callback));
  }

  std::string test_name;
  bool seed_verification;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const ChangePasswordPasswordInitTestCase*
ChangePasswordPasswordInitCalledInWrongState() {
  static const base::NoDestructor<ChangePasswordPasswordInitTestCase>
      kChangePasswordPasswordInitCalledInWrongState({
          .test_name = "change_password_password_init_called_in_wrong_state",
          .seed_verification = false,
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::ChangePasswordError::NewClientError(
                  mojom::ChangePasswordClientError::New(
                      mojom::ChangePasswordClientErrorCode::
                          kCalledInWrongState))),
      });
  return kChangePasswordPasswordInitCalledInWrongState.get();
}

const ChangePasswordPasswordInitTestCase*
ChangePasswordPasswordInitVerificationTokenDecryptionFailed() {
  static const base::NoDestructor<ChangePasswordPasswordInitTestCase>
      kChangePasswordPasswordInitVerificationTokenDecryptionFailed({
          .test_name = "change_password_password_init_verification_token_"
                       "decryption_failed",
          .seed_verification = true,
          .fail_decryption = true,
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::ChangePasswordError::NewClientError(
                  mojom::ChangePasswordClientError::New(
                      mojom::ChangePasswordClientErrorCode::
                          kVerificationTokenDecryptionFailed))),
      });
  return kChangePasswordPasswordInitVerificationTokenDecryptionFailed.get();
}

const ChangePasswordPasswordInitTestCase*
ChangePasswordPasswordInitBodyMissingOrFailedToParse() {
  static const base::NoDestructor<ChangePasswordPasswordInitTestCase>
      kChangePasswordPasswordInitBodyMissingOrFailedToParse({
          .test_name =
              "change_password_password_init_body_missing_or_failed_to_parse",
          .seed_verification = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .mojo_expected =
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_INTERNAL_SERVER_ERROR,
                      mojom::ChangePasswordServerErrorCode::kInvalidResponse))),
      });
  return kChangePasswordPasswordInitBodyMissingOrFailedToParse.get();
}

const ChangePasswordPasswordInitTestCase*
ChangePasswordPasswordInitErrorCodeIsNull() {
  static const base::NoDestructor<ChangePasswordPasswordInitTestCase>
      kChangePasswordPasswordInitErrorCodeIsNull({
          .test_name = "change_password_password_init_error_code_is_null",
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
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::ChangePasswordServerErrorCode::kNull))),
      });
  return kChangePasswordPasswordInitErrorCodeIsNull.get();
}

const ChangePasswordPasswordInitTestCase*
ChangePasswordPasswordInitUnknownErrorCode() {
  static const base::NoDestructor<ChangePasswordPasswordInitTestCase>
      kChangePasswordPasswordInitUnknownErrorCode({
          .test_name = "change_password_password_init_unknown_error_code",
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
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_TOO_EARLY,
                      mojom::ChangePasswordServerErrorCode::kUnknown))),
      });
  return kChangePasswordPasswordInitUnknownErrorCode.get();
}

const ChangePasswordPasswordInitTestCase*
ChangePasswordPasswordInitKnownErrorCode() {
  static const base::NoDestructor<ChangePasswordPasswordInitTestCase>
      kChangePasswordPasswordInitKnownErrorCode({
          .test_name = "change_password_password_init_known_error_code",
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
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::ChangePasswordServerErrorCode::
                          kTooManyVerifications))),
      });
  return kChangePasswordPasswordInitKnownErrorCode.get();
}

const ChangePasswordPasswordInitTestCase*
ChangePasswordPasswordInitSerializedResponseEmpty() {
  static const base::NoDestructor<ChangePasswordPasswordInitTestCase>
      kChangePasswordPasswordInitSerializedResponseEmpty({
          .test_name =
              "change_password_password_init_serialized_response_empty",
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
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_OK,
                      mojom::ChangePasswordServerErrorCode::kInvalidResponse))),
      });
  return kChangePasswordPasswordInitSerializedResponseEmpty.get();
}

const ChangePasswordPasswordInitTestCase* ChangePasswordPasswordInitSuccess() {
  static const base::NoDestructor<ChangePasswordPasswordInitTestCase>
      kChangePasswordPasswordInitSuccess({
          .test_name = "change_password_password_init_success",
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
          .mojo_expected = mojom::ChangePasswordPasswordInitResult::New(
              "serialized_response"),
      });
  return kChangePasswordPasswordInitSuccess.get();
}

using BraveAccountServiceChangePasswordPasswordInitTest =
    BraveAccountServiceTest<ChangePasswordPasswordInitTestCase>;

}  // namespace

TEST_P(BraveAccountServiceChangePasswordPasswordInitTest,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceChangePasswordPasswordInitTest,
    testing::Values(
        ChangePasswordPasswordInitCalledInWrongState(),
        ChangePasswordPasswordInitVerificationTokenDecryptionFailed(),
        ChangePasswordPasswordInitBodyMissingOrFailedToParse(),
        ChangePasswordPasswordInitErrorCodeIsNull(),
        ChangePasswordPasswordInitUnknownErrorCode(),
        ChangePasswordPasswordInitKnownErrorCode(),
        ChangePasswordPasswordInitSerializedResponseEmpty(),
        ChangePasswordPasswordInitSuccess()),
    BraveAccountServiceChangePasswordPasswordInitTest::kNameGenerator);

struct ChangePasswordPasswordFinalizeTestCase {
  using Endpoint = PasswordFinalize;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected =
      base::expected<mojom::ChangePasswordPasswordFinalizeResultPtr,
                     mojom::ChangePasswordErrorPtr>;

  static void Run(const ChangePasswordPasswordFinalizeTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    SeedLoggedIn(pref_service, test_case.seed_verification);

    authentication->ChangePasswordPasswordFinalize(
        "serialized_record",
        std::move(callback).Then(base::BindOnce(
            [](PrefService* pref_service, bool success) {
              AccountStatePrefs account_state_prefs(*pref_service);
              const auto state = account_state_prefs.GetAccountState();
              ASSERT_TRUE(state->is_logged_in());
              // The logged-in email and authentication token are left intact.
              EXPECT_EQ(state->get_logged_in()->email, kEmailAddress);
              EXPECT_EQ(account_state_prefs.GetAuthenticationToken(),
                        EncryptedAuthenticationToken());
              if (success) {
                // Verification slot dropped.
                EXPECT_FALSE(state->get_logged_in()->verification);
              }
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

const ChangePasswordPasswordFinalizeTestCase*
ChangePasswordPasswordFinalizeCalledInWrongState() {
  static const base::NoDestructor<ChangePasswordPasswordFinalizeTestCase>
      kChangePasswordPasswordFinalizeCalledInWrongState({
          .test_name =
              "change_password_password_finalize_called_in_wrong_state",
          .seed_verification = false,
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::ChangePasswordError::NewClientError(
                  mojom::ChangePasswordClientError::New(
                      mojom::ChangePasswordClientErrorCode::
                          kCalledInWrongState))),
      });
  return kChangePasswordPasswordFinalizeCalledInWrongState.get();
}

const ChangePasswordPasswordFinalizeTestCase*
ChangePasswordPasswordFinalizeVerificationTokenDecryptionFailed() {
  static const base::NoDestructor<ChangePasswordPasswordFinalizeTestCase>
      kChangePasswordPasswordFinalizeVerificationTokenDecryptionFailed({
          .test_name = "change_password_password_finalize_verification_token_"
                       "decryption_failed",
          .seed_verification = true,
          .fail_decryption = true,
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::ChangePasswordError::NewClientError(
                  mojom::ChangePasswordClientError::New(
                      mojom::ChangePasswordClientErrorCode::
                          kVerificationTokenDecryptionFailed))),
      });
  return kChangePasswordPasswordFinalizeVerificationTokenDecryptionFailed.get();
}

const ChangePasswordPasswordFinalizeTestCase*
ChangePasswordPasswordFinalizeBodyMissingOrFailedToParse() {
  static const base::NoDestructor<ChangePasswordPasswordFinalizeTestCase>
      kChangePasswordPasswordFinalizeBodyMissingOrFailedToParse({
          .test_name =
              "change_password_password_finalize_body_missing_or_failed_to_"
              "parse",
          .seed_verification = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .mojo_expected =
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_INTERNAL_SERVER_ERROR,
                      mojom::ChangePasswordServerErrorCode::kInvalidResponse))),
      });
  return kChangePasswordPasswordFinalizeBodyMissingOrFailedToParse.get();
}

const ChangePasswordPasswordFinalizeTestCase*
ChangePasswordPasswordFinalizeErrorCodeIsNull() {
  static const base::NoDestructor<ChangePasswordPasswordFinalizeTestCase>
      kChangePasswordPasswordFinalizeErrorCodeIsNull({
          .test_name = "change_password_password_finalize_error_code_is_null",
          .seed_verification = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_NOT_FOUND,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_NOT_FOUND,
                      mojom::ChangePasswordServerErrorCode::kNull))),
      });
  return kChangePasswordPasswordFinalizeErrorCodeIsNull.get();
}

const ChangePasswordPasswordFinalizeTestCase*
ChangePasswordPasswordFinalizeUnknownErrorCode() {
  static const base::NoDestructor<ChangePasswordPasswordFinalizeTestCase>
      kChangePasswordPasswordFinalizeUnknownErrorCode({
          .test_name = "change_password_password_finalize_unknown_error_code",
          .seed_verification = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_TOO_EARLY,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value(42);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_TOO_EARLY,
                      mojom::ChangePasswordServerErrorCode::kUnknown))),
      });
  return kChangePasswordPasswordFinalizeUnknownErrorCode.get();
}

const ChangePasswordPasswordFinalizeTestCase*
ChangePasswordPasswordFinalizeKnownErrorCode() {
  static const base::NoDestructor<ChangePasswordPasswordFinalizeTestCase>
      kChangePasswordPasswordFinalizeKnownErrorCode({
          .test_name = "change_password_password_finalize_known_error_code",
          .seed_verification = true,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_NOT_FOUND,
                                 .body = base::unexpected([] {
                                   PasswordFinalize::Response::ErrorBody body;
                                   body.code = base::Value(14001);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::ChangePasswordError::NewServerError(
                  mojom::ChangePasswordServerError::New(
                      net::HTTP_NOT_FOUND,
                      mojom::ChangePasswordServerErrorCode::
                          kInterimPasswordStateNotFound))),
      });
  return kChangePasswordPasswordFinalizeKnownErrorCode.get();
}

const ChangePasswordPasswordFinalizeTestCase*
ChangePasswordPasswordFinalizeSuccess() {
  static const base::NoDestructor<ChangePasswordPasswordFinalizeTestCase>
      kChangePasswordPasswordFinalizeSuccess({
          .test_name = "change_password_password_finalize_success",
          .seed_verification = true,
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
          .mojo_expected = mojom::ChangePasswordPasswordFinalizeResult::New(),
      });
  return kChangePasswordPasswordFinalizeSuccess.get();
}

using BraveAccountServiceChangePasswordPasswordFinalizeTest =
    BraveAccountServiceTest<ChangePasswordPasswordFinalizeTestCase>;

}  // namespace

TEST_P(BraveAccountServiceChangePasswordPasswordFinalizeTest,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceChangePasswordPasswordFinalizeTest,
    testing::Values(
        ChangePasswordPasswordFinalizeCalledInWrongState(),
        ChangePasswordPasswordFinalizeVerificationTokenDecryptionFailed(),
        ChangePasswordPasswordFinalizeBodyMissingOrFailedToParse(),
        ChangePasswordPasswordFinalizeErrorCodeIsNull(),
        ChangePasswordPasswordFinalizeUnknownErrorCode(),
        ChangePasswordPasswordFinalizeKnownErrorCode(),
        ChangePasswordPasswordFinalizeSuccess()),
    BraveAccountServiceChangePasswordPasswordFinalizeTest::kNameGenerator);

}  // namespace brave_account
