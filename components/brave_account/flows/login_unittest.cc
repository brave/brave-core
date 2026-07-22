/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/login.h"

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
#include "brave/components/brave_account/endpoints/login_finalize.h"
#include "brave/components/brave_account/endpoints/login_init.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/mojom/login.mojom.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/http/http_status_code.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

using endpoints::LoginFinalize;
using endpoints::LoginInit;

struct LoginStep1TestCase {
  using Endpoint = LoginInit;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected =
      base::expected<mojom::LoginStep1ResultPtr, mojom::LoginErrorPtr>;

  static void Run(const LoginStep1TestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    authentication->LoginStep1(mojom::Service::kAccounts, kEmailAddress,
                               "serialized_ke1", std::move(callback));
  }

  std::string test_name;
  bool fail_encryption;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const LoginStep1TestCase* LoginStep1BodyMissingOrFailedToParse() {
  static const base::NoDestructor<LoginStep1TestCase>
      kLoginStep1BodyMissingOrFailedToParse({
          .test_name = "login_step1_body_missing_or_failed_to_parse",
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::NewServerError(mojom::LoginServerError::New(
                  net::HTTP_INTERNAL_SERVER_ERROR,
                  mojom::LoginServerErrorCode::kInvalidResponse))),
      });
  return kLoginStep1BodyMissingOrFailedToParse.get();
}

const LoginStep1TestCase* LoginStep1ErrorCodeIsNull() {
  static const base::NoDestructor<LoginStep1TestCase>
      kLoginStep1ErrorCodeIsNull({
          .test_name = "login_step1_error_code_is_null",
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   LoginInit::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::NewServerError(mojom::LoginServerError::New(
                  net::HTTP_BAD_REQUEST, mojom::LoginServerErrorCode::kNull))),
      });
  return kLoginStep1ErrorCodeIsNull.get();
}

const LoginStep1TestCase* LoginStep1UnknownErrorCode() {
  static const base::NoDestructor<LoginStep1TestCase>
      kLoginStep1UnknownErrorCode({
          .test_name = "login_step1_unknown_error_code",
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_TOO_EARLY,
                                 .body = base::unexpected([] {
                                   LoginInit::Response::ErrorBody body;
                                   body.code = base::Value(42);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::NewServerError(mojom::LoginServerError::New(
                  net::HTTP_TOO_EARLY, mojom::LoginServerErrorCode::kUnknown))),
      });
  return kLoginStep1UnknownErrorCode.get();
}

const LoginStep1TestCase* LoginStep1KnownErrorCode() {
  static const base::NoDestructor<LoginStep1TestCase> kLoginStep1KnownErrorCode(
      {
          .test_name = "login_step1_known_error_code",
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   LoginInit::Response::ErrorBody body;
                                   body.code = base::Value(11003);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::NewServerError(mojom::LoginServerError::New(
                  net::HTTP_UNAUTHORIZED,
                  mojom::LoginServerErrorCode::kEmailNotVerified))),
      });
  return kLoginStep1KnownErrorCode.get();
}

const LoginStep1TestCase* LoginStep1LoginTokenEmpty() {
  static const base::NoDestructor<LoginStep1TestCase>
      kLoginStep1LoginTokenEmpty({
          .test_name = "login_step1_login_token_empty",
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       LoginInit::Response::SuccessBody body;
                                       body.login_token = "";
                                       body.serialized_ke2 = "serialized_ke2";
                                       return body;
                                     }()}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::NewServerError(mojom::LoginServerError::New(
                  net::HTTP_OK,
                  mojom::LoginServerErrorCode::kInvalidResponse))),
      });
  return kLoginStep1LoginTokenEmpty.get();
}

const LoginStep1TestCase* LoginStep1SerializedKe2Empty() {
  static const base::NoDestructor<LoginStep1TestCase>
      kLoginStep1SerializedKe2Empty({
          .test_name = "login_step1_serialized_ke2_empty",
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       LoginInit::Response::SuccessBody body;
                                       body.login_token = kLoginToken;
                                       body.serialized_ke2 = "";
                                       return body;
                                     }()}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::NewServerError(mojom::LoginServerError::New(
                  net::HTTP_OK,
                  mojom::LoginServerErrorCode::kInvalidResponse))),
      });
  return kLoginStep1SerializedKe2Empty.get();
}

const LoginStep1TestCase* LoginStep1LoginTokenEncryptionFailed() {
  static const base::NoDestructor<LoginStep1TestCase>
      kLoginStep1LoginTokenEncryptionFailed({
          .test_name = "login_step1_login_token_encryption_failed",
          .fail_encryption = true,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       LoginInit::Response::SuccessBody body;
                                       body.login_token = kLoginToken;
                                       body.serialized_ke2 = "serialized_ke2";
                                       return body;
                                     }()}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::NewClientError(mojom::LoginClientError::New(
                  mojom::LoginClientErrorCode::kLoginTokenEncryptionFailed))),
      });
  return kLoginStep1LoginTokenEncryptionFailed.get();
}

const LoginStep1TestCase* LoginStep1Success() {
  static const base::NoDestructor<LoginStep1TestCase> kLoginStep1Success({
      .test_name = "login_step1_success",
      .fail_encryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   LoginInit::Response::SuccessBody body;
                                   body.login_token = kLoginToken;
                                   body.serialized_ke2 = "serialized_ke2";
                                   return body;
                                 }()}},
      .mojo_expected =
          mojom::LoginStep1Result::New(EncryptedLoginToken(), "serialized_ke2"),
  });
  return kLoginStep1Success.get();
}

using BraveAccountServiceLoginStep1Test =
    BraveAccountServiceTest<LoginStep1TestCase>;

}  // namespace

TEST_P(BraveAccountServiceLoginStep1Test, MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(BraveAccountServiceTests,
                         BraveAccountServiceLoginStep1Test,
                         testing::Values(LoginStep1BodyMissingOrFailedToParse(),
                                         LoginStep1ErrorCodeIsNull(),
                                         LoginStep1UnknownErrorCode(),
                                         LoginStep1KnownErrorCode(),
                                         LoginStep1LoginTokenEmpty(),
                                         LoginStep1SerializedKe2Empty(),
                                         LoginStep1LoginTokenEncryptionFailed(),
                                         LoginStep1Success()),
                         BraveAccountServiceLoginStep1Test::kNameGenerator);

struct LoginStep2TestCase {
  using Endpoint = LoginFinalize;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected =
      base::expected<mojom::LoginStep2ResultPtr, mojom::LoginErrorPtr>;

  static void Run(const LoginStep2TestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    authentication->LoginStep2(
        EncryptedLoginToken(), "client_mac",
        std::move(callback).Then(base::BindOnce(
            [](PrefService* pref_service, bool success) {
              AccountStatePrefs account_state_prefs(*pref_service);
              const auto state = account_state_prefs.GetAccountState();
              if (success) {
                ASSERT_TRUE(state->is_logged_in());
                EXPECT_EQ(state->get_logged_in()->email, kEmailAddress);
                EXPECT_EQ(account_state_prefs.GetAuthenticationToken(),
                          EncryptedAuthenticationToken());
              } else {
                EXPECT_TRUE(state->is_logged_out());
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

const LoginStep2TestCase* LoginStep2LoginTokenDecryptionFailed() {
  static const base::NoDestructor<LoginStep2TestCase>
      kLoginStep2LoginTokenDecryptionFailed({
          .test_name = "login_step2_login_token_decryption_failed",
          .fail_decryption = true,
          .fail_encryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected = base::unexpected(
              mojom::LoginError::NewClientError(mojom::LoginClientError::New(
                  mojom::LoginClientErrorCode::kLoginTokenDecryptionFailed))),
      });
  return kLoginStep2LoginTokenDecryptionFailed.get();
}

const LoginStep2TestCase* LoginStep2BodyMissingOrFailedToParse() {
  static const base::NoDestructor<LoginStep2TestCase>
      kLoginStep2BodyMissingOrFailedToParse({
          .test_name = "login_step2_body_missing_or_failed_to_parse",
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::NewServerError(mojom::LoginServerError::New(
                  net::HTTP_INTERNAL_SERVER_ERROR,
                  mojom::LoginServerErrorCode::kInvalidResponse))),
      });
  return kLoginStep2BodyMissingOrFailedToParse.get();
}

const LoginStep2TestCase* LoginStep2ErrorCodeIsNull() {
  static const base::NoDestructor<LoginStep2TestCase>
      kLoginStep2ErrorCodeIsNull({
          .test_name = "login_step2_error_code_is_null",
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   LoginFinalize::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::NewServerError(mojom::LoginServerError::New(
                  net::HTTP_BAD_REQUEST, mojom::LoginServerErrorCode::kNull))),
      });
  return kLoginStep2ErrorCodeIsNull.get();
}

const LoginStep2TestCase* LoginStep2UnknownErrorCode() {
  static const base::NoDestructor<LoginStep2TestCase>
      kLoginStep2UnknownErrorCode({
          .test_name = "login_step2_unknown_error_code",
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_TOO_EARLY,
                                 .body = base::unexpected([] {
                                   LoginFinalize::Response::ErrorBody body;
                                   body.code = base::Value(42);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::NewServerError(mojom::LoginServerError::New(
                  net::HTTP_TOO_EARLY, mojom::LoginServerErrorCode::kUnknown))),
      });
  return kLoginStep2UnknownErrorCode.get();
}

const LoginStep2TestCase* LoginStep2KnownErrorCode() {
  static const base::NoDestructor<LoginStep2TestCase> kLoginStep2KnownErrorCode(
      {
          .test_name = "login_step2_known_error_code",
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   LoginFinalize::Response::ErrorBody body;
                                   body.code = base::Value(14009);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::NewServerError(mojom::LoginServerError::New(
                  net::HTTP_BAD_REQUEST,
                  mojom::LoginServerErrorCode::kInterimPasswordStateMismatch))),
      });
  return kLoginStep2KnownErrorCode.get();
}

const LoginStep2TestCase* LoginStep2AuthTokenEmpty() {
  static const base::NoDestructor<LoginStep2TestCase> kLoginStep2AuthTokenEmpty(
      {
          .test_name = "login_step2_auth_token_empty",
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       LoginFinalize::Response::SuccessBody
                                           body;
                                       body.auth_token = "";
                                       body.email = kEmailAddress;
                                       return body;
                                     }()}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::NewServerError(mojom::LoginServerError::New(
                  net::HTTP_OK,
                  mojom::LoginServerErrorCode::kInvalidResponse))),
      });
  return kLoginStep2AuthTokenEmpty.get();
}

const LoginStep2TestCase* LoginStep2EmailEmpty() {
  static const base::NoDestructor<LoginStep2TestCase> kLoginStep2EmailEmpty({
      .test_name = "login_step2_email_empty",
      .fail_decryption = false,
      .fail_encryption = {},  // not used
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   LoginFinalize::Response::SuccessBody body;
                                   body.auth_token = kAuthenticationToken;
                                   body.email = "";
                                   return body;
                                 }()}},
      .mojo_expected = base::unexpected(
          mojom::LoginError::NewServerError(mojom::LoginServerError::New(
              net::HTTP_OK, mojom::LoginServerErrorCode::kInvalidResponse))),
  });
  return kLoginStep2EmailEmpty.get();
}

const LoginStep2TestCase* LoginStep2AuthenticationTokenEncryptionFailed() {
  static const base::NoDestructor<LoginStep2TestCase>
      kLoginStep2AuthenticationTokenEncryptionFailed({
          .test_name = "login_step2_authentication_token_"
                       "encryption_failed",
          .fail_decryption = false,
          .fail_encryption = true,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       LoginFinalize::Response::SuccessBody
                                           body;
                                       body.auth_token = kAuthenticationToken;
                                       body.email = kEmailAddress;
                                       return body;
                                     }()}},
          .mojo_expected = base::unexpected(
              mojom::LoginError::NewClientError(mojom::LoginClientError::New(
                  mojom::LoginClientErrorCode::
                      kAuthenticationTokenEncryptionFailed))),
      });
  return kLoginStep2AuthenticationTokenEncryptionFailed.get();
}

const LoginStep2TestCase* LoginStep2Success() {
  static const base::NoDestructor<LoginStep2TestCase> kLoginStep2Success({
      .test_name = "login_step2_success",
      .fail_decryption = false,
      .fail_encryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   LoginFinalize::Response::SuccessBody body;
                                   body.auth_token = kAuthenticationToken;
                                   body.email = kEmailAddress;
                                   return body;
                                 }()}},
      .mojo_expected = mojom::LoginStep2Result::New(),
  });
  return kLoginStep2Success.get();
}

using BraveAccountServiceLoginStep2Test =
    BraveAccountServiceTest<LoginStep2TestCase>;

}  // namespace

TEST_P(BraveAccountServiceLoginStep2Test, MapsEndpointExpectedToMojoExpected) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceLoginStep2Test,
    testing::Values(LoginStep2LoginTokenDecryptionFailed(),
                    LoginStep2BodyMissingOrFailedToParse(),
                    LoginStep2ErrorCodeIsNull(),
                    LoginStep2UnknownErrorCode(),
                    LoginStep2KnownErrorCode(),
                    LoginStep2AuthTokenEmpty(),
                    LoginStep2EmailEmpty(),
                    LoginStep2AuthenticationTokenEncryptionFailed(),
                    LoginStep2Success()),
    BraveAccountServiceLoginStep2Test::kNameGenerator);

}  // namespace brave_account
