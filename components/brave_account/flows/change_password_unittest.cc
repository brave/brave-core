/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/change_password.h"

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
// after `ChangePasswordStep1` additionally require the verification slot
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

struct ChangePasswordStep1TestCase {
  using Endpoint = VerifyInit;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::ChangePasswordStep1ResultPtr,
                                      mojom::ChangePasswordErrorPtr>;

  static void Run(const ChangePasswordStep1TestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    SeedLoggedIn(pref_service);

    authentication->ChangePasswordStep1(
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

const ChangePasswordStep1TestCase*
ChangePasswordStep1AuthenticationTokenDecryptionFailed() {
  static const base::NoDestructor<ChangePasswordStep1TestCase> kTestCase({
      .test_name = "change_password_step1_authentication_token_"
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
  return kTestCase.get();
}

const ChangePasswordStep1TestCase*
ChangePasswordStep1BodyMissingOrFailedToParse() {
  static const base::NoDestructor<ChangePasswordStep1TestCase> kTestCase({
      .test_name = "change_password_step1_body_missing_or_failed_to_parse",
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
  return kTestCase.get();
}

const ChangePasswordStep1TestCase* ChangePasswordStep1ErrorCodeIsNull() {
  static const base::NoDestructor<ChangePasswordStep1TestCase> kTestCase({
      .test_name = "change_password_step1_error_code_is_null",
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
  return kTestCase.get();
}

const ChangePasswordStep1TestCase* ChangePasswordStep1UnknownErrorCode() {
  static const base::NoDestructor<ChangePasswordStep1TestCase> kTestCase({
      .test_name = "change_password_step1_unknown_error_code",
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
  return kTestCase.get();
}

const ChangePasswordStep1TestCase* ChangePasswordStep1KnownErrorCode() {
  static const base::NoDestructor<ChangePasswordStep1TestCase> kTestCase({
      .test_name = "change_password_step1_known_error_code",
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
                  net::HTTP_BAD_REQUEST, mojom::ChangePasswordServerErrorCode::
                                             kTooManyVerifications))),
  });
  return kTestCase.get();
}

const ChangePasswordStep1TestCase* ChangePasswordStep1VerificationTokenEmpty() {
  static const base::NoDestructor<ChangePasswordStep1TestCase> kTestCase({
      .test_name = "change_password_step1_verification_token_empty",
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
  return kTestCase.get();
}

const ChangePasswordStep1TestCase*
ChangePasswordStep1VerificationTokenEncryptionFailed() {
  static const base::NoDestructor<ChangePasswordStep1TestCase> kTestCase({
      .test_name = "change_password_step1_verification_token_"
                   "encryption_failed",
      .fail_decryption = false,
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
          base::unexpected(mojom::ChangePasswordError::NewClientError(
              mojom::ChangePasswordClientError::New(
                  mojom::ChangePasswordClientErrorCode::
                      kVerificationTokenEncryptionFailed))),
  });
  return kTestCase.get();
}

const ChangePasswordStep1TestCase* ChangePasswordStep1Success() {
  static const base::NoDestructor<ChangePasswordStep1TestCase> kTestCase({
      .test_name = "change_password_step1_success",
      .fail_decryption = false,
      .fail_encryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   VerifyInit::Response::SuccessBody body;
                                   body.verification_token = kVerificationToken;
                                   return body;
                                 }()}},
      .mojo_expected = mojom::ChangePasswordStep1Result::New(),
  });
  return kTestCase.get();
}

using BraveAccountServiceChangePasswordStep1Test =
    BraveAccountServiceTest<ChangePasswordStep1TestCase>;

}  // namespace

TEST_P(BraveAccountServiceChangePasswordStep1Test,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceChangePasswordStep1Test,
    testing::Values(ChangePasswordStep1AuthenticationTokenDecryptionFailed(),
                    ChangePasswordStep1BodyMissingOrFailedToParse(),
                    ChangePasswordStep1ErrorCodeIsNull(),
                    ChangePasswordStep1UnknownErrorCode(),
                    ChangePasswordStep1KnownErrorCode(),
                    ChangePasswordStep1VerificationTokenEmpty(),
                    ChangePasswordStep1VerificationTokenEncryptionFailed(),
                    ChangePasswordStep1Success()),
    BraveAccountServiceChangePasswordStep1Test::kNameGenerator);

struct ChangePasswordStep2TestCase {
  using Endpoint = VerifyComplete;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::ChangePasswordStep2ResultPtr,
                                      mojom::ChangePasswordErrorPtr>;

  static void Run(const ChangePasswordStep2TestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    SeedLoggedIn(pref_service, test_case.seed_verification);

    authentication->ChangePasswordStep2(
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

const ChangePasswordStep2TestCase* ChangePasswordStep2CalledInWrongState() {
  static const base::NoDestructor<ChangePasswordStep2TestCase> kTestCase({
      .test_name = "change_password_step2_called_in_wrong_state",
      .seed_verification = false,
      .fail_decryption = {},    // not used
      .endpoint_response = {},  // not used
      .mojo_expected =
          base::unexpected(mojom::ChangePasswordError::NewClientError(
              mojom::ChangePasswordClientError::New(
                  mojom::ChangePasswordClientErrorCode::kCalledInWrongState))),
  });
  return kTestCase.get();
}

const ChangePasswordStep2TestCase*
ChangePasswordStep2VerificationTokenDecryptionFailed() {
  static const base::NoDestructor<ChangePasswordStep2TestCase> kTestCase({
      .test_name = "change_password_step2_verification_token_"
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
  return kTestCase.get();
}

const ChangePasswordStep2TestCase*
ChangePasswordStep2BodyMissingOrFailedToParse() {
  static const base::NoDestructor<ChangePasswordStep2TestCase> kTestCase({
      .test_name = "change_password_step2_body_missing_or_failed_to_parse",
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
  return kTestCase.get();
}

const ChangePasswordStep2TestCase* ChangePasswordStep2ErrorCodeIsNull() {
  static const base::NoDestructor<ChangePasswordStep2TestCase> kTestCase({
      .test_name = "change_password_step2_error_code_is_null",
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
  return kTestCase.get();
}

const ChangePasswordStep2TestCase* ChangePasswordStep2UnknownErrorCode() {
  static const base::NoDestructor<ChangePasswordStep2TestCase> kTestCase({
      .test_name = "change_password_step2_unknown_error_code",
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
  return kTestCase.get();
}

const ChangePasswordStep2TestCase* ChangePasswordStep2KnownErrorCode() {
  static const base::NoDestructor<ChangePasswordStep2TestCase> kTestCase({
      .test_name = "change_password_step2_known_error_code",
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
                  net::HTTP_BAD_REQUEST, mojom::ChangePasswordServerErrorCode::
                                             kInvalidVerificationCode))),
  });
  return kTestCase.get();
}

const ChangePasswordStep2TestCase* ChangePasswordStep2EmailEmpty() {
  static const base::NoDestructor<ChangePasswordStep2TestCase> kTestCase({
      .test_name = "change_password_step2_email_empty",
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
          base::unexpected(mojom::ChangePasswordError::NewServerError(
              mojom::ChangePasswordServerError::New(
                  net::HTTP_OK,
                  mojom::ChangePasswordServerErrorCode::kInvalidResponse))),
  });
  return kTestCase.get();
}

const ChangePasswordStep2TestCase* ChangePasswordStep2Success() {
  static const base::NoDestructor<ChangePasswordStep2TestCase> kTestCase({
      .test_name = "change_password_step2_success",
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
      .mojo_expected = mojom::ChangePasswordStep2Result::New(),
  });
  return kTestCase.get();
}

using BraveAccountServiceChangePasswordStep2Test =
    BraveAccountServiceTest<ChangePasswordStep2TestCase>;

}  // namespace

TEST_P(BraveAccountServiceChangePasswordStep2Test,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceChangePasswordStep2Test,
    testing::Values(ChangePasswordStep2CalledInWrongState(),
                    ChangePasswordStep2VerificationTokenDecryptionFailed(),
                    ChangePasswordStep2BodyMissingOrFailedToParse(),
                    ChangePasswordStep2ErrorCodeIsNull(),
                    ChangePasswordStep2UnknownErrorCode(),
                    ChangePasswordStep2KnownErrorCode(),
                    ChangePasswordStep2EmailEmpty(),
                    ChangePasswordStep2Success()),
    BraveAccountServiceChangePasswordStep2Test::kNameGenerator);

struct ChangePasswordStep3TestCase {
  using Endpoint = PasswordInit;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::ChangePasswordStep3ResultPtr,
                                      mojom::ChangePasswordErrorPtr>;

  static void Run(const ChangePasswordStep3TestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    SeedLoggedIn(pref_service, test_case.seed_verification);

    authentication->ChangePasswordStep3("blinded_message", std::move(callback));
  }

  std::string test_name;
  bool seed_verification;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const ChangePasswordStep3TestCase* ChangePasswordStep3CalledInWrongState() {
  static const base::NoDestructor<ChangePasswordStep3TestCase> kTestCase({
      .test_name = "change_password_step3_called_in_wrong_state",
      .seed_verification = false,
      .fail_decryption = {},    // not used
      .endpoint_response = {},  // not used
      .mojo_expected =
          base::unexpected(mojom::ChangePasswordError::NewClientError(
              mojom::ChangePasswordClientError::New(
                  mojom::ChangePasswordClientErrorCode::kCalledInWrongState))),
  });
  return kTestCase.get();
}

const ChangePasswordStep3TestCase*
ChangePasswordStep3VerificationTokenDecryptionFailed() {
  static const base::NoDestructor<ChangePasswordStep3TestCase> kTestCase({
      .test_name = "change_password_step3_verification_token_"
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
  return kTestCase.get();
}

const ChangePasswordStep3TestCase*
ChangePasswordStep3BodyMissingOrFailedToParse() {
  static const base::NoDestructor<ChangePasswordStep3TestCase> kTestCase({
      .test_name = "change_password_step3_body_missing_or_failed_to_parse",
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
  return kTestCase.get();
}

const ChangePasswordStep3TestCase* ChangePasswordStep3ErrorCodeIsNull() {
  static const base::NoDestructor<ChangePasswordStep3TestCase> kTestCase({
      .test_name = "change_password_step3_error_code_is_null",
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
  return kTestCase.get();
}

const ChangePasswordStep3TestCase* ChangePasswordStep3UnknownErrorCode() {
  static const base::NoDestructor<ChangePasswordStep3TestCase> kTestCase({
      .test_name = "change_password_step3_unknown_error_code",
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
  return kTestCase.get();
}

const ChangePasswordStep3TestCase* ChangePasswordStep3KnownErrorCode() {
  static const base::NoDestructor<ChangePasswordStep3TestCase> kTestCase({
      .test_name = "change_password_step3_known_error_code",
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
                  net::HTTP_BAD_REQUEST, mojom::ChangePasswordServerErrorCode::
                                             kTooManyVerifications))),
  });
  return kTestCase.get();
}

const ChangePasswordStep3TestCase*
ChangePasswordStep3SerializedResponseEmpty() {
  static const base::NoDestructor<ChangePasswordStep3TestCase> kTestCase({
      .test_name = "change_password_step3_serialized_response_empty",
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
  return kTestCase.get();
}

const ChangePasswordStep3TestCase* ChangePasswordStep3Success() {
  static const base::NoDestructor<ChangePasswordStep3TestCase> kTestCase({
      .test_name = "change_password_step3_success",
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
          mojom::ChangePasswordStep3Result::New("serialized_response"),
  });
  return kTestCase.get();
}

using BraveAccountServiceChangePasswordStep3Test =
    BraveAccountServiceTest<ChangePasswordStep3TestCase>;

}  // namespace

TEST_P(BraveAccountServiceChangePasswordStep3Test,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceChangePasswordStep3Test,
    testing::Values(ChangePasswordStep3CalledInWrongState(),
                    ChangePasswordStep3VerificationTokenDecryptionFailed(),
                    ChangePasswordStep3BodyMissingOrFailedToParse(),
                    ChangePasswordStep3ErrorCodeIsNull(),
                    ChangePasswordStep3UnknownErrorCode(),
                    ChangePasswordStep3KnownErrorCode(),
                    ChangePasswordStep3SerializedResponseEmpty(),
                    ChangePasswordStep3Success()),
    BraveAccountServiceChangePasswordStep3Test::kNameGenerator);

struct ChangePasswordStep4TestCase {
  using Endpoint = PasswordFinalize;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::ChangePasswordStep4ResultPtr,
                                      mojom::ChangePasswordErrorPtr>;

  static void Run(const ChangePasswordStep4TestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    SeedLoggedIn(pref_service, test_case.seed_verification);

    authentication->ChangePasswordStep4(
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

const ChangePasswordStep4TestCase* ChangePasswordStep4CalledInWrongState() {
  static const base::NoDestructor<ChangePasswordStep4TestCase> kTestCase({
      .test_name = "change_password_step4_called_in_wrong_state",
      .seed_verification = false,
      .fail_decryption = {},    // not used
      .endpoint_response = {},  // not used
      .mojo_expected =
          base::unexpected(mojom::ChangePasswordError::NewClientError(
              mojom::ChangePasswordClientError::New(
                  mojom::ChangePasswordClientErrorCode::kCalledInWrongState))),
  });
  return kTestCase.get();
}

const ChangePasswordStep4TestCase*
ChangePasswordStep4VerificationTokenDecryptionFailed() {
  static const base::NoDestructor<ChangePasswordStep4TestCase> kTestCase({
      .test_name = "change_password_step4_verification_token_"
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
  return kTestCase.get();
}

const ChangePasswordStep4TestCase*
ChangePasswordStep4BodyMissingOrFailedToParse() {
  static const base::NoDestructor<ChangePasswordStep4TestCase> kTestCase({
      .test_name = "change_password_step4_body_missing_or_failed_to_"
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
  return kTestCase.get();
}

const ChangePasswordStep4TestCase* ChangePasswordStep4ErrorCodeIsNull() {
  static const base::NoDestructor<ChangePasswordStep4TestCase> kTestCase({
      .test_name = "change_password_step4_error_code_is_null",
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
  return kTestCase.get();
}

const ChangePasswordStep4TestCase* ChangePasswordStep4UnknownErrorCode() {
  static const base::NoDestructor<ChangePasswordStep4TestCase> kTestCase({
      .test_name = "change_password_step4_unknown_error_code",
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
  return kTestCase.get();
}

const ChangePasswordStep4TestCase* ChangePasswordStep4KnownErrorCode() {
  static const base::NoDestructor<ChangePasswordStep4TestCase> kTestCase({
      .test_name = "change_password_step4_known_error_code",
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
                  net::HTTP_NOT_FOUND, mojom::ChangePasswordServerErrorCode::
                                           kInterimPasswordStateNotFound))),
  });
  return kTestCase.get();
}

const ChangePasswordStep4TestCase* ChangePasswordStep4Success() {
  static const base::NoDestructor<ChangePasswordStep4TestCase> kTestCase({
      .test_name = "change_password_step4_success",
      .seed_verification = true,
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   PasswordFinalize::Response::SuccessBody body;
                                   body.auth_token =
                                       base::Value(kAuthenticationToken);
                                   return body;
                                 }()}},
      .mojo_expected = mojom::ChangePasswordStep4Result::New(),
  });
  return kTestCase.get();
}

using BraveAccountServiceChangePasswordStep4Test =
    BraveAccountServiceTest<ChangePasswordStep4TestCase>;

}  // namespace

TEST_P(BraveAccountServiceChangePasswordStep4Test,
       MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceChangePasswordStep4Test,
    testing::Values(ChangePasswordStep4CalledInWrongState(),
                    ChangePasswordStep4VerificationTokenDecryptionFailed(),
                    ChangePasswordStep4BodyMissingOrFailedToParse(),
                    ChangePasswordStep4ErrorCodeIsNull(),
                    ChangePasswordStep4UnknownErrorCode(),
                    ChangePasswordStep4KnownErrorCode(),
                    ChangePasswordStep4Success()),
    BraveAccountServiceChangePasswordStep4Test::kNameGenerator);

}  // namespace brave_account
