/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_service.h"

#include <optional>
#include <string>

#include "base/base64.h"
#include "base/functional/callback.h"
#include "base/json/values_util.h"
#include "base/no_destructor.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_account/brave_account_service_constants.h"
#include "brave/components/brave_account/brave_account_service_test.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoints/auth_validate.h"
#include "brave/components/brave_account/endpoints/service_token.h"
#include "brave/components/brave_account/endpoints/verify_resend.h"
#include "brave/components/brave_account/mock_brave_account_authentication_observer.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"
#include "net/http/http_status_code.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

using endpoints::AuthValidate;
using endpoints::ServiceToken;
using endpoints::VerifyResend;

struct AuthenticationObserverTestCase {
  enum class StateAction {
    kSwitchToVerification,
    kSwitchToLoggedIn,
    kSwitchToLoggedOut,
    kUpdateEmailAddress,
  };

  static void Run(const AuthenticationObserverTestCase& test_case,
                  PrefService& pref_service,
                  mojo::Remote<mojom::Authentication>& authentication) {
    const auto account_state_eq = [](const mojom::AccountStatePtr& expected) {
      return testing::Truly([&](const mojom::AccountStatePtr& state) {
        return state.Equals(expected);
      });
    };

    AccountStatePrefs account_state_prefs(pref_service);

    switch (CHECK_DEREF(test_case.from).which()) {
      case mojom::AccountState::Tag::kLoggedOut:
        if (test_case.from->get_logged_out()->verification) {
          account_state_prefs.AddVerification(
              EncryptedVerificationToken(),
              mojom::VerificationIntent::NewLoggedOutIntent(
                  mojom::LoggedOutVerificationIntent::kRegistration));
        }
        break;
      case mojom::AccountState::Tag::kLoggedIn:
        account_state_prefs.SetLoggedIn(kEmailAddress,
                                        EncryptedAuthenticationToken());
        break;
    }

    MockBraveAccountAuthenticationObserver mock_observer;
    // Observer should receive initial state when added.
    EXPECT_CALL(mock_observer,
                OnAccountStateChanged(account_state_eq(test_case.from)))
        .Times(1);

    authentication->AddObserver(mock_observer.BindAndGetRemote());
    mock_observer.FlushForTesting();
    testing::Mock::VerifyAndClearExpectations(&mock_observer);

    EXPECT_CALL(mock_observer,
                OnAccountStateChanged(account_state_eq(test_case.to)))
        .Times(1);

    switch (test_case.action) {
      case StateAction::kSwitchToVerification:
        account_state_prefs.AddVerification(
            EncryptedVerificationToken(),
            mojom::VerificationIntent::NewLoggedOutIntent(
                mojom::LoggedOutVerificationIntent::kRegistration));
        break;
      case StateAction::kSwitchToLoggedIn:
        account_state_prefs.SetLoggedIn(kEmailAddress,
                                        EncryptedAuthenticationToken());
        break;
      case StateAction::kSwitchToLoggedOut:
        account_state_prefs.SetLoggedOut();
        break;
      case StateAction::kUpdateEmailAddress:
        account_state_prefs.UpdateEmail("new_email");
        break;
    }

    mock_observer.FlushForTesting();
  }

  std::string test_name;
  mojom::AccountStatePtr from;
  StateAction action;
  mojom::AccountStatePtr to;
};

namespace {

mojom::AccountStatePtr LoggedOut() {
  return mojom::AccountState::NewLoggedOut(mojom::LoggedOutState::New(nullptr));
}

mojom::AccountStatePtr LoggedOutWithVerification() {
  return mojom::AccountState::NewLoggedOut(
      mojom::LoggedOutState::New(mojom::LoggedOutVerification::New(
          mojom::LoggedOutVerificationIntent::kRegistration, /*email=*/"")));
}

mojom::AccountStatePtr LoggedIn(const std::string& email) {
  return mojom::AccountState::NewLoggedIn(
      mojom::LoggedInState::New(email, nullptr));
}

const AuthenticationObserverTestCase*
AuthenticationObserverLoggedOutToVerification() {
  static const base::NoDestructor<AuthenticationObserverTestCase>
      kAuthenticationObserverLoggedOutToVerification(
          {.test_name = "authentication_observer_logged_out_to_verification",
           .from = LoggedOut(),
           .action = AuthenticationObserverTestCase::StateAction::
               kSwitchToVerification,
           .to = LoggedOutWithVerification()});
  return kAuthenticationObserverLoggedOutToVerification.get();
}

const AuthenticationObserverTestCase*
AuthenticationObserverVerificationToLoggedIn() {
  static const base::NoDestructor<AuthenticationObserverTestCase>
      kAuthenticationObserverVerificationToLoggedIn(
          {.test_name = "authentication_observer_verification_to_logged_in",
           .from = LoggedOutWithVerification(),
           .action =
               AuthenticationObserverTestCase::StateAction::kSwitchToLoggedIn,
           .to = LoggedIn(kEmailAddress)});
  return kAuthenticationObserverVerificationToLoggedIn.get();
}

const AuthenticationObserverTestCase*
AuthenticationObserverLoggedInToLoggedOut() {
  static const base::NoDestructor<AuthenticationObserverTestCase>
      kAuthenticationObserverLoggedInToLoggedOut(
          {.test_name = "authentication_observer_logged_in_to_logged_out",
           .from = LoggedIn(kEmailAddress),
           .action =
               AuthenticationObserverTestCase::StateAction::kSwitchToLoggedOut,
           .to = LoggedOut()});
  return kAuthenticationObserverLoggedInToLoggedOut.get();
}

const AuthenticationObserverTestCase*
AuthenticationObserverLoggedOutToLoggedIn() {
  static const base::NoDestructor<AuthenticationObserverTestCase>
      kAuthenticationObserverLoggedOutToLoggedIn(
          {.test_name = "authentication_observer_logged_out_to_logged_in",
           .from = LoggedOut(),
           .action =
               AuthenticationObserverTestCase::StateAction::kSwitchToLoggedIn,
           .to = LoggedIn(kEmailAddress)});
  return kAuthenticationObserverLoggedOutToLoggedIn.get();
}

const AuthenticationObserverTestCase*
AuthenticationObserverLoggedInToLoggedInEmailChange() {
  static const base::NoDestructor<AuthenticationObserverTestCase>
      kAuthenticationObserverLoggedInToLoggedInEmailChange(
          {.test_name =
               "authentication_observer_logged_in_to_logged_in_email_change",
           .from = LoggedIn(kEmailAddress),
           .action =
               AuthenticationObserverTestCase::StateAction::kUpdateEmailAddress,
           .to = LoggedIn("new_email")});
  return kAuthenticationObserverLoggedInToLoggedInEmailChange.get();
}

using BraveAccountServiceAuthenticationObserverTest =
    BraveAccountServiceTest<AuthenticationObserverTestCase>;

}  // namespace

TEST_P(BraveAccountServiceAuthenticationObserverTest, OnAccountStateChanged) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceAuthenticationObserverTest,
    testing::Values(AuthenticationObserverLoggedOutToVerification(),
                    AuthenticationObserverVerificationToLoggedIn(),
                    AuthenticationObserverLoggedInToLoggedOut(),
                    AuthenticationObserverLoggedOutToLoggedIn(),
                    AuthenticationObserverLoggedInToLoggedInEmailChange()),
    BraveAccountServiceAuthenticationObserverTest::kNameGenerator);

struct ResendVerificationEmailTestCase {
  using Endpoint = VerifyResend;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::ResendConfirmationEmailResultPtr,
                                      mojom::ResendConfirmationEmailErrorPtr>;

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
ResendVerificationEmailVerificationTokenEmpty() {
  static const base::NoDestructor<ResendVerificationEmailTestCase>
      kResendVerificationEmailVerificationTokenEmpty({
          .test_name = "resend_confirmation_email_verification_token_empty",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kResetPassword,
          .fail_decryption = {},    // not used
          .endpoint_response = {},  // not used
          .mojo_expected = base::unexpected(
              mojom::ResendConfirmationEmailError::NewClientError(
                  mojom::ResendConfirmationEmailClientError::New(
                      mojom::ResendConfirmationEmailClientErrorCode::
                          kCalledInWrongState))),
      });
  return kResendVerificationEmailVerificationTokenEmpty.get();
}

const ResendVerificationEmailTestCase*
ResendVerificationEmailVerificationTokenFailedToDecrypt() {
  static const base::NoDestructor<ResendVerificationEmailTestCase>
      kResendVerificationEmailVerificationTokenFailedToDecrypt({
          .test_name =
              "resend_confirmation_email_verification_token_failed_to_decrypt",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kRegistration,
          .fail_decryption = true,
          .endpoint_response = {},  // not used
          .mojo_expected = base::unexpected(
              mojom::ResendConfirmationEmailError::NewClientError(
                  mojom::ResendConfirmationEmailClientError::New(
                      mojom::ResendConfirmationEmailClientErrorCode::
                          kVerificationTokenDecryptionFailed))),
      });
  return kResendVerificationEmailVerificationTokenFailedToDecrypt.get();
}

const ResendVerificationEmailTestCase* ResendVerificationEmailSuccess() {
  static const base::NoDestructor<ResendVerificationEmailTestCase>
      kResendVerificationEmailSuccess({
          .test_name = "resend_confirmation_email_success",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kRegistration,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_NO_CONTENT,
                                 .body = std::nullopt}},
          .mojo_expected = mojom::ResendConfirmationEmailResult::New(),
      });
  return kResendVerificationEmailSuccess.get();
}

const ResendVerificationEmailTestCase* ResendVerificationEmailNetworkError() {
  static const base::NoDestructor<ResendVerificationEmailTestCase>
      kResendVerificationEmailNetworkError({
          .test_name = "resend_confirmation_email_network_error",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kRegistration,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::ERR_CONNECTION_REFUSED,
                                 .status_code = std::nullopt,
                                 .body = std::nullopt}},
          .mojo_expected = base::unexpected(
              mojom::ResendConfirmationEmailError::NewServerError(
                  mojom::ResendConfirmationEmailServerError::New(
                      net::ERR_CONNECTION_REFUSED,
                      mojom::ResendConfirmationEmailServerErrorCode::
                          kInvalidResponse))),
      });
  return kResendVerificationEmailNetworkError.get();
}

const ResendVerificationEmailTestCase*
ResendVerificationEmailBodyMissingOrFailedToParse() {
  static const base::NoDestructor<ResendVerificationEmailTestCase>
      kResendVerificationEmailBodyMissingOrFailedToParse({
          .test_name =
              "resend_confirmation_email_body_missing_or_failed_to_parse",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kRegistration,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .mojo_expected = base::unexpected(
              mojom::ResendConfirmationEmailError::NewServerError(
                  mojom::ResendConfirmationEmailServerError::New(
                      net::HTTP_INTERNAL_SERVER_ERROR,
                      mojom::ResendConfirmationEmailServerErrorCode::
                          kInvalidResponse))),
      });
  return kResendVerificationEmailBodyMissingOrFailedToParse.get();
}

const ResendVerificationEmailTestCase*
ResendVerificationEmailBadRequestWithNullErrorCode() {
  static const base::NoDestructor<ResendVerificationEmailTestCase>
      kResendVerificationEmailBadRequestWithNullErrorCode({
          .test_name =
              "resend_confirmation_email_bad_request_with_null_error_code",
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
          .mojo_expected = base::unexpected(
              mojom::ResendConfirmationEmailError::NewServerError(
                  mojom::ResendConfirmationEmailServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::ResendConfirmationEmailServerErrorCode::kNull))),
      });
  return kResendVerificationEmailBadRequestWithNullErrorCode.get();
}

const ResendVerificationEmailTestCase*
ResendVerificationEmailMaximumEmailSendAttemptsExceeded() {
  static const base::NoDestructor<ResendVerificationEmailTestCase>
      kResendVerificationEmailMaximumEmailSendAttemptsExceeded({
          .test_name =
              "resend_confirmation_email_maximum_email_send_attempts_exceeded",
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
          .mojo_expected = base::unexpected(
              mojom::ResendConfirmationEmailError::NewServerError(
                  mojom::ResendConfirmationEmailServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::ResendConfirmationEmailServerErrorCode::
                          kMaximumEmailSendAttemptsExceeded))),
      });
  return kResendVerificationEmailMaximumEmailSendAttemptsExceeded.get();
}

const ResendVerificationEmailTestCase*
ResendVerificationEmailEmailAlreadyVerified() {
  static const base::NoDestructor<ResendVerificationEmailTestCase>
      kResendVerificationEmailEmailAlreadyVerified({
          .test_name = "resend_confirmation_email_email_already_verified",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kRegistration,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   VerifyResend::Response::ErrorBody body;
                                   body.code = base::Value(13009);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(
              mojom::ResendConfirmationEmailError::NewServerError(
                  mojom::ResendConfirmationEmailServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::ResendConfirmationEmailServerErrorCode::
                          kEmailAlreadyVerified))),
      });
  return kResendVerificationEmailEmailAlreadyVerified.get();
}

const ResendVerificationEmailTestCase*
ResendVerificationEmailTokenHasExpired() {
  static const base::NoDestructor<ResendVerificationEmailTestCase>
      kResendVerificationEmailTokenHasExpired({
          .test_name = "resend_verification_email_token_has_expired",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kRegistration,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   VerifyResend::Response::ErrorBody body;
                                   body.code = base::Value(14014);
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(
              mojom::ResendConfirmationEmailError::NewServerError(
                  mojom::ResendConfirmationEmailServerError::New(
                      net::HTTP_UNAUTHORIZED,
                      mojom::ResendConfirmationEmailServerErrorCode::
                          kTokenHasExpired))),
      });
  return kResendVerificationEmailTokenHasExpired.get();
}

const ResendVerificationEmailTestCase* ResendVerificationEmailServerError() {
  static const base::NoDestructor<ResendVerificationEmailTestCase>
      kResendVerificationEmailServerError({
          .test_name = "resend_confirmation_email_server_error",
          .logged_out_verification_intent =
              mojom::LoggedOutVerificationIntent::kRegistration,
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = base::unexpected([] {
                                   VerifyResend::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .mojo_expected = base::unexpected(
              mojom::ResendConfirmationEmailError::NewServerError(
                  mojom::ResendConfirmationEmailServerError::New(
                      net::HTTP_INTERNAL_SERVER_ERROR,
                      mojom::ResendConfirmationEmailServerErrorCode::kNull))),
      });
  return kResendVerificationEmailServerError.get();
}

const ResendVerificationEmailTestCase* ResendVerificationEmailUnknown() {
  static const base::NoDestructor<ResendVerificationEmailTestCase>
      kResendVerificationEmailUnknown({
          .test_name = "resend_confirmation_email_unknown",
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
          .mojo_expected = base::unexpected(
              mojom::ResendConfirmationEmailError::NewServerError(
                  mojom::ResendConfirmationEmailServerError::New(
                      net::HTTP_TOO_EARLY,
                      mojom::ResendConfirmationEmailServerErrorCode::
                          kUnknown))),
      });
  return kResendVerificationEmailUnknown.get();
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
    testing::Values(ResendVerificationEmailVerificationTokenEmpty(),
                    ResendVerificationEmailVerificationTokenFailedToDecrypt(),
                    ResendVerificationEmailSuccess(),
                    ResendVerificationEmailNetworkError(),
                    ResendVerificationEmailBodyMissingOrFailedToParse(),
                    ResendVerificationEmailBadRequestWithNullErrorCode(),
                    ResendVerificationEmailMaximumEmailSendAttemptsExceeded(),
                    ResendVerificationEmailEmailAlreadyVerified(),
                    ResendVerificationEmailTokenHasExpired(),
                    ResendVerificationEmailServerError(),
                    ResendVerificationEmailUnknown()),
    BraveAccountServiceResendVerificationEmailTest::kNameGenerator);

struct AuthValidateTestCase {
  using Endpoint = AuthValidate;
  using EndpointResponse = Endpoint::Response;

  static void Run(const AuthValidateTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  BraveAccountService& brave_account_service) {
    AccountStatePrefs account_state_prefs(pref_service);
    account_state_prefs.SetLoggedIn(kEmailAddress,
                                    EncryptedAuthenticationToken());

    task_environment.FastForwardBy(kAuthValidatePollInterval -
                                   base::Seconds(1));

    const auto state = account_state_prefs.GetAccountState();
    if (test_case.expected_authentication_token.empty()) {
      EXPECT_TRUE(state->is_logged_out());
    } else {
      ASSERT_TRUE(state->is_logged_in());
      EXPECT_EQ(state->get_logged_in()->email, test_case.expected_email);
      EXPECT_EQ(account_state_prefs.GetAuthenticationToken(),
                test_case.expected_authentication_token);
    }

    base::OneShotTimer* auth_validate_timer =
        brave_account_service.AuthValidateTimerForTesting();
    if (test_case.expected_auth_validate_timer_delay.is_zero()) {
      if (auth_validate_timer) {
        EXPECT_FALSE(auth_validate_timer->IsRunning());
      }
    } else {
      ASSERT_TRUE(auth_validate_timer);
      EXPECT_TRUE(auth_validate_timer->IsRunning());
      EXPECT_EQ(auth_validate_timer->GetCurrentDelay(),
                test_case.expected_auth_validate_timer_delay);
    }
  }

  std::string test_name;
  bool fail_decryption;
  std::optional<EndpointResponse> endpoint_response;
  std::string expected_email;
  std::string expected_authentication_token;
  base::TimeDelta expected_auth_validate_timer_delay;
};

namespace {

const AuthValidateTestCase* AuthValidateAuthenticationTokenFailedToDecrypt() {
  static const base::NoDestructor<AuthValidateTestCase>
      kAuthValidateAuthenticationTokenFailedToDecrypt({
          .test_name = "auth_validate_authentication_token_failed_to_decrypt",
          .fail_decryption = true,
          .endpoint_response = {},  // not used
          .expected_email = kEmailAddress,
          .expected_authentication_token = EncryptedAuthenticationToken(),
          .expected_auth_validate_timer_delay = {},
      });
  return kAuthValidateAuthenticationTokenFailedToDecrypt.get();
}

const AuthValidateTestCase* AuthValidateSuccessNoBody() {
  static const base::NoDestructor<AuthValidateTestCase>
      kAuthValidateSuccessNoBody({
          .test_name = "auth_validate_success_no_body",
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body = {}}},
          .expected_email = kEmailAddress,
          .expected_authentication_token = EncryptedAuthenticationToken(),
          .expected_auth_validate_timer_delay = kAuthValidatePollInterval,
      });
  return kAuthValidateSuccessNoBody.get();
}

const AuthValidateTestCase* AuthValidateSuccessEmailEmpty() {
  static const base::NoDestructor<AuthValidateTestCase>
      kAuthValidateSuccessEmailEmpty({
          .test_name = "auth_validate_success_email_empty",
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       AuthValidate::Response::SuccessBody body;
                                       body.email = "";
                                       return body;
                                     }()}},
          .expected_email = kEmailAddress,
          .expected_authentication_token = EncryptedAuthenticationToken(),
          .expected_auth_validate_timer_delay = kAuthValidatePollInterval,
      });
  return kAuthValidateSuccessEmailEmpty.get();
}

const AuthValidateTestCase* AuthValidateSuccess() {
  static const base::NoDestructor<AuthValidateTestCase> kAuthValidateSuccess({
      .test_name = "auth_validate_success",
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_OK,
                             .body =
                                 [] {
                                   AuthValidate::Response::SuccessBody body;
                                   body.email = "email_from_response";
                                   return body;
                                 }()}},
      .expected_email = "email_from_response",
      .expected_authentication_token = EncryptedAuthenticationToken(),
      .expected_auth_validate_timer_delay = kAuthValidatePollInterval,
  });
  return kAuthValidateSuccess.get();
}

const AuthValidateTestCase* AuthValidateUnauthorized() {
  static const base::NoDestructor<AuthValidateTestCase>
      kAuthValidateUnauthorized({
          .test_name = "auth_validate_unauthorized",
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   AuthValidate::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .expected_email = "",
          .expected_authentication_token = "",
          .expected_auth_validate_timer_delay = {},
      });
  return kAuthValidateUnauthorized.get();
}

const AuthValidateTestCase* AuthValidateForbidden() {
  static const base::NoDestructor<AuthValidateTestCase> kAuthValidateForbidden({
      .test_name = "auth_validate_forbidden",
      .fail_decryption = false,
      .endpoint_response = {{.net_error = net::OK,
                             .status_code = net::HTTP_FORBIDDEN,
                             .body = base::unexpected([] {
                               AuthValidate::Response::ErrorBody body;
                               body.code = base::Value(14007);
                               return body;
                             }())}},
      .expected_email = "",
      .expected_authentication_token = "",
      .expected_auth_validate_timer_delay = {},
  });
  return kAuthValidateForbidden.get();
}

const AuthValidateTestCase* AuthValidateInternalServerError() {
  static const base::NoDestructor<AuthValidateTestCase>
      kAuthValidateInternalServerError({
          .test_name = "auth_validate_internal_server_error",
          .fail_decryption = false,
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = base::unexpected([] {
                                   AuthValidate::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .expected_email = kEmailAddress,
          .expected_authentication_token = EncryptedAuthenticationToken(),
          .expected_auth_validate_timer_delay = kAuthValidatePollInterval,
      });
  return kAuthValidateInternalServerError.get();
}

using BraveAccountServiceScheduleAuthValidateTest =
    BraveAccountServiceTest<AuthValidateTestCase>;

}  // namespace

TEST_P(BraveAccountServiceScheduleAuthValidateTest,
       HandlesAuthValidateOutcomes) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceScheduleAuthValidateTest,
    testing::Values(AuthValidateAuthenticationTokenFailedToDecrypt(),
                    AuthValidateSuccessNoBody(),
                    AuthValidateSuccessEmailEmpty(),
                    AuthValidateSuccess(),
                    AuthValidateUnauthorized(),
                    AuthValidateForbidden(),
                    AuthValidateInternalServerError()),
    BraveAccountServiceScheduleAuthValidateTest::kNameGenerator);

struct CancelVerificationTestCase {
  static void Run(const CancelVerificationTestCase& test_case,
                  PrefService& pref_service,
                  mojo::Remote<mojom::Authentication>& authentication) {
    AccountStatePrefs account_state_prefs(pref_service);
    account_state_prefs.AddVerification(
        EncryptedVerificationToken(),
        mojom::VerificationIntent::NewLoggedOutIntent(
            mojom::LoggedOutVerificationIntent::kRegistration));
    authentication->CancelVerification(
        mojom::VerificationIntent::NewLoggedOutIntent(
            mojom::LoggedOutVerificationIntent::kRegistration));
    authentication.FlushForTesting();
    const auto state = account_state_prefs.GetAccountState();
    ASSERT_TRUE(state->is_logged_out());
    EXPECT_FALSE(state->get_logged_out()->verification);
  }

  std::string test_name;
};

namespace {

const CancelVerificationTestCase*
CancelVerificationVerificationTokenNonEmpty() {
  static const base::NoDestructor<CancelVerificationTestCase>
      kCancelVerificationVerificationTokenNonEmpty({
          .test_name = "cancel_registration_verification_token_non_empty",
      });
  return kCancelVerificationVerificationTokenNonEmpty.get();
}

using BraveAccountServiceCancelVerificationTest =
    BraveAccountServiceTest<CancelVerificationTestCase>;

}  // namespace

TEST_P(BraveAccountServiceCancelVerificationTest,
       HandlesCancelVerificationOutcomes) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceCancelVerificationTest,
    testing::Values(CancelVerificationVerificationTokenNonEmpty()),
    BraveAccountServiceCancelVerificationTest::kNameGenerator);

struct LogOutTestCase {
  static void Run(const LogOutTestCase& test_case,
                  PrefService& pref_service,
                  mojo::Remote<mojom::Authentication>& authentication) {
    AccountStatePrefs account_state_prefs(pref_service);
    account_state_prefs.SetLoggedIn(kEmailAddress,
                                    EncryptedAuthenticationToken());
    authentication->LogOut();
    authentication.FlushForTesting();
    const auto state = account_state_prefs.GetAccountState();
    ASSERT_TRUE(state->is_logged_out());
    EXPECT_FALSE(state->get_logged_out()->verification);
  }

  std::string test_name;
};

namespace {

const LogOutTestCase* LogOutAuthenticationTokenNonEmpty() {
  static const base::NoDestructor<LogOutTestCase>
      kLogOutAuthenticationTokenNonEmpty({
          .test_name = "log_out_authentication_token_non_empty",
      });
  return kLogOutAuthenticationTokenNonEmpty.get();
}

using BraveAccountServiceLogOutTest = BraveAccountServiceTest<LogOutTestCase>;

}  // namespace

TEST_P(BraveAccountServiceLogOutTest, HandlesLogOutOutcomes) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(BraveAccountServiceTests,
                         BraveAccountServiceLogOutTest,
                         testing::Values(LogOutAuthenticationTokenNonEmpty()),
                         BraveAccountServiceLogOutTest::kNameGenerator);

struct GetServiceTokenTestCase {
  using Endpoint = ServiceToken;
  using EndpointResponse = Endpoint::Response;
  using MojoExpected = base::expected<mojom::GetServiceTokenResultPtr,
                                      mojom::GetServiceTokenErrorPtr>;

  static void Run(const GetServiceTokenTestCase& test_case,
                  PrefService& pref_service,
                  base::test::TaskEnvironment& task_environment,
                  mojo::Remote<mojom::Authentication>& authentication,
                  base::OnceCallback<void(MojoExpected)> callback) {
    AccountStatePrefs(pref_service)
        .SetLoggedIn(kEmailAddress, EncryptedAuthenticationToken());
    ScopedDictPrefUpdate(&pref_service, prefs::kBraveAccountState)
        ->Set(prefs::keys::kServiceTokens,
              std::move(test_case.service_tokens_dict).Run(base::Time::Now()));

    task_environment.FastForwardBy(test_case.time_advance);

    std::string expected_service_token =
        test_case.mojo_expected.has_value()
            ? test_case.mojo_expected.value()->serviceToken
            : "";

    authentication->GetServiceToken(
        mojom::Service::kEmailAliases,
        // |callback| resolves the TestFuture in BraveAccountServiceTest with
        // the result. The Then() callback runs immediately after, before the
        // test verifies the future's value.
        std::move(callback).Then(base::BindOnce(
            [](PrefService* pref_service, std::string expected_service_token) {
              if (!expected_service_token.empty()) {
                const auto* service_tokens =
                    pref_service->GetDict(prefs::kBraveAccountState)
                        .FindDict(prefs::keys::kServiceTokens);
                ASSERT_TRUE(service_tokens);
                const auto* email_aliases =
                    service_tokens->FindDict("email-aliases");
                ASSERT_TRUE(email_aliases);
                const auto* service_token =
                    email_aliases->FindString(prefs::keys::kServiceToken);
                ASSERT_TRUE(service_token);
                EXPECT_EQ(*service_token,
                          base::Base64Encode(expected_service_token));
              }
            },
            base::Unretained(&pref_service),
            std::move(expected_service_token))));
  }

  std::string test_name;
  // |service_tokens_dict| is a callback instead of a plain base::DictValue
  // so that test cases can use the current mock time (passed as parameter)
  // when constructing the dictionary. This is necessary for cache expiration
  // tests that need to set timestamps relative to when the test runs.
  mutable base::OnceCallback<base::DictValue(base::Time)> service_tokens_dict;
  bool fail_decryption;
  bool fail_encryption;
  base::TimeDelta time_advance;
  std::optional<EndpointResponse> endpoint_response;
  MojoExpected mojo_expected;
};

namespace {

const GetServiceTokenTestCase* GetServiceTokenCacheHit() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenCacheHit({
          .test_name = "get_service_token_cache_hit",
          .service_tokens_dict = base::BindOnce([](base::Time mock_now) {
            return base::DictValue().Set(
                "email-aliases",
                base::DictValue()
                    .Set(prefs::keys::kServiceToken,
                         base::Base64Encode("cached_service_token"))
                    .Set(prefs::keys::kLastFetched,
                         base::TimeToValue(mock_now)));
          }),
          .fail_decryption = {},    // not used
          .fail_encryption = {},    // not used
          .time_advance = {},       // not used
          .endpoint_response = {},  // not used
          .mojo_expected =
              mojom::GetServiceTokenResult::New("cached_service_token"),
      });
  return kGetServiceTokenCacheHit.get();
}

const GetServiceTokenTestCase*
GetServiceTokenAuthenticationTokenDecryptionFailed() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenAuthenticationTokenDecryptionFailed({
          .test_name =
              "get_service_token_authentication_token_decryption_failed",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .fail_decryption = true,
          .fail_encryption = {},    // not used
          .time_advance = {},       // not used
          .endpoint_response = {},  // not used
          .mojo_expected =
              base::unexpected(mojom::GetServiceTokenError::NewClientError(
                  mojom::GetServiceTokenClientError::New(
                      mojom::GetServiceTokenClientErrorCode::
                          kAuthenticationTokenDecryptionFailed))),
      });
  return kGetServiceTokenAuthenticationTokenDecryptionFailed.get();
}

const GetServiceTokenTestCase* GetServiceTokenNetworkError() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenNetworkError({
          .test_name = "get_service_token_network_error",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::ERR_CONNECTION_REFUSED,
                                 .status_code = std::nullopt,
                                 .body = std::nullopt}},
          .mojo_expected =
              base::unexpected(mojom::GetServiceTokenError::NewServerError(
                  mojom::GetServiceTokenServerError::New(
                      net::ERR_CONNECTION_REFUSED,
                      mojom::GetServiceTokenServerErrorCode::
                          kInvalidResponse))),
      });
  return kGetServiceTokenNetworkError.get();
}

const GetServiceTokenTestCase* GetServiceTokenBodyMissingOrFailedToParse() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenBodyMissingOrFailedToParse({
          .test_name = "get_service_token_body_missing_or_failed_to_parse",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = std::nullopt}},
          .mojo_expected =
              base::unexpected(mojom::GetServiceTokenError::NewServerError(
                  mojom::GetServiceTokenServerError::New(
                      net::HTTP_INTERNAL_SERVER_ERROR,
                      mojom::GetServiceTokenServerErrorCode::
                          kInvalidResponse))),
      });
  return kGetServiceTokenBodyMissingOrFailedToParse.get();
}

const GetServiceTokenTestCase* GetServiceTokenErrorCodeIsNull() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenErrorCodeIsNull({
          .test_name = "get_service_token_error_code_is_null",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   ServiceToken::Response::ErrorBody body;
                                   body.code = base::Value();
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::GetServiceTokenError::NewServerError(
                  mojom::GetServiceTokenServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::GetServiceTokenServerErrorCode::kNull))),
      });
  return kGetServiceTokenErrorCodeIsNull.get();
}

const GetServiceTokenTestCase* GetServiceTokenEmailDomainNotSupported() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenEmailDomainNotSupported({
          .test_name = "get_service_token_email_domain_not_supported",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   ServiceToken::Response::ErrorBody body;
                                   body.code = base::Value(13006);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::GetServiceTokenError::NewServerError(
                  mojom::GetServiceTokenServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::GetServiceTokenServerErrorCode::
                          kEmailDomainNotSupported))),
      });
  return kGetServiceTokenEmailDomainNotSupported.get();
}

const GetServiceTokenTestCase* GetServiceTokenIncorrectCredentials() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenIncorrectCredentials({
          .test_name = "get_service_token_incorrect_credentials",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_FORBIDDEN,
                                 .body = base::unexpected([] {
                                   ServiceToken::Response::ErrorBody body;
                                   body.code = base::Value(14004);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::GetServiceTokenError::NewServerError(
                  mojom::GetServiceTokenServerError::New(
                      net::HTTP_FORBIDDEN,
                      mojom::GetServiceTokenServerErrorCode::
                          kIncorrectCredentials))),
      });
  return kGetServiceTokenIncorrectCredentials.get();
}

const GetServiceTokenTestCase* GetServiceTokenInvalidTokenAudience() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenInvalidTokenAudience({
          .test_name = "get_service_token_invalid_token_audience",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_FORBIDDEN,
                                 .body = base::unexpected([] {
                                   ServiceToken::Response::ErrorBody body;
                                   body.code = base::Value(14007);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::GetServiceTokenError::NewServerError(
                  mojom::GetServiceTokenServerError::New(
                      net::HTTP_FORBIDDEN,
                      mojom::GetServiceTokenServerErrorCode::
                          kInvalidTokenAudience))),
      });
  return kGetServiceTokenInvalidTokenAudience.get();
}

const GetServiceTokenTestCase* GetServiceTokenBadRequest() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenBadRequest({
          .test_name = "get_service_token_bad_request",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_BAD_REQUEST,
                                 .body = base::unexpected([] {
                                   ServiceToken::Response::ErrorBody body;
                                   body.code = base::Value(0);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::GetServiceTokenError::NewServerError(
                  mojom::GetServiceTokenServerError::New(
                      net::HTTP_BAD_REQUEST,
                      mojom::GetServiceTokenServerErrorCode::kNull))),
      });
  return kGetServiceTokenBadRequest.get();
}

const GetServiceTokenTestCase* GetServiceTokenUnauthorized() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenUnauthorized({
          .test_name = "get_service_token_unauthorized",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_UNAUTHORIZED,
                                 .body = base::unexpected([] {
                                   ServiceToken::Response::ErrorBody body;
                                   body.code = base::Value(0);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::GetServiceTokenError::NewServerError(
                  mojom::GetServiceTokenServerError::New(
                      net::HTTP_UNAUTHORIZED,
                      mojom::GetServiceTokenServerErrorCode::kNull))),
      });
  return kGetServiceTokenUnauthorized.get();
}

const GetServiceTokenTestCase* GetServiceTokenServerError() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenInternalServerError({
          .test_name = "get_service_token_internal_server_error",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_INTERNAL_SERVER_ERROR,
                                 .body = base::unexpected([] {
                                   ServiceToken::Response::ErrorBody body;
                                   body.code = base::Value(0);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::GetServiceTokenError::NewServerError(
                  mojom::GetServiceTokenServerError::New(
                      net::HTTP_INTERNAL_SERVER_ERROR,
                      mojom::GetServiceTokenServerErrorCode::kNull))),
      });
  return kGetServiceTokenInternalServerError.get();
}

const GetServiceTokenTestCase* GetServiceTokenUnknown() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenUnknown({
          .test_name = "get_service_token_unknown",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_TOO_EARLY,
                                 .body = base::unexpected([] {
                                   ServiceToken::Response::ErrorBody body;
                                   body.code = base::Value(42);
                                   return body;
                                 }())}},
          .mojo_expected =
              base::unexpected(mojom::GetServiceTokenError::NewServerError(
                  mojom::GetServiceTokenServerError::New(
                      net::HTTP_TOO_EARLY,
                      mojom::GetServiceTokenServerErrorCode::kUnknown))),
      });
  return kGetServiceTokenUnknown.get();
}

const GetServiceTokenTestCase* GetServiceTokenServiceTokenEmpty() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenServiceTokenEmpty({
          .test_name = "get_service_token_service_token_empty",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .fail_decryption = false,
          .fail_encryption = {},  // not used
          .time_advance = {},     // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token = "";
                                       return body;
                                     }()}},
          .mojo_expected =
              base::unexpected(mojom::GetServiceTokenError::NewServerError(
                  mojom::GetServiceTokenServerError::New(
                      net::HTTP_OK, mojom::GetServiceTokenServerErrorCode::
                                        kInvalidResponse))),
      });
  return kGetServiceTokenServiceTokenEmpty.get();
}

const GetServiceTokenTestCase* GetServiceTokenServiceTokenEncryptionFailed() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenServiceTokenEncryptionFailed({
          .test_name = "get_service_token_service_token_encryption_failed",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .fail_decryption = false,
          .fail_encryption = true,
          .time_advance = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token =
                                           "fetched_service_token";
                                       return body;
                                     }()}},
          .mojo_expected =
              base::unexpected(mojom::GetServiceTokenError::NewClientError(
                  mojom::GetServiceTokenClientError::New(
                      mojom::GetServiceTokenClientErrorCode::
                          kServiceTokenEncryptionFailed))),
      });
  return kGetServiceTokenServiceTokenEncryptionFailed.get();
}

const GetServiceTokenTestCase* GetServiceTokenSuccess() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenSuccess({
          .test_name = "get_service_token_success",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .fail_decryption = false,
          .fail_encryption = false,
          .time_advance = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token =
                                           "fetched_service_token";
                                       return body;
                                     }()}},
          .mojo_expected =
              mojom::GetServiceTokenResult::New("fetched_service_token"),
      });
  return kGetServiceTokenSuccess.get();
}

const GetServiceTokenTestCase* GetServiceTokenServiceDictMissing() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenServiceDictMissing({
          .test_name = "get_service_token_service_dict_missing",
          .service_tokens_dict =
              base::BindOnce([](base::Time) { return base::DictValue(); }),
          .fail_decryption = false,
          .fail_encryption = false,
          .time_advance = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token =
                                           "fetched_service_token";
                                       return body;
                                     }()}},
          .mojo_expected =
              mojom::GetServiceTokenResult::New("fetched_service_token"),
      });
  return kGetServiceTokenServiceDictMissing.get();
}

const GetServiceTokenTestCase* GetServiceTokenServiceTokenMissing() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenServiceTokenMissing({
          .test_name = "get_service_token_cache_service_token_missing",
          .service_tokens_dict = base::BindOnce([](base::Time mock_now) {
            return base::DictValue().Set(
                "email-aliases",
                base::DictValue().Set(prefs::keys::kLastFetched,
                                      base::TimeToValue(mock_now)));
          }),
          .fail_decryption = false,
          .fail_encryption = false,
          .time_advance = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token =
                                           "fetched_service_token";
                                       return body;
                                     }()}},
          .mojo_expected =
              mojom::GetServiceTokenResult::New("fetched_service_token"),
      });
  return kGetServiceTokenServiceTokenMissing.get();
}

const GetServiceTokenTestCase* GetServiceTokenLastFetchedMissing() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenLastFetchedMissing({
          .test_name = "get_service_token_cache_last_fetched_missing",
          .service_tokens_dict = base::BindOnce([](base::Time) {
            return base::DictValue().Set(
                "email-aliases",
                base::DictValue().Set(
                    prefs::keys::kServiceToken,
                    base::Base64Encode("cached_service_token")));
          }),
          .fail_decryption = false,
          .fail_encryption = false,
          .time_advance = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token =
                                           "fetched_service_token";
                                       return body;
                                     }()}},
          .mojo_expected =
              mojom::GetServiceTokenResult::New("fetched_service_token"),
      });
  return kGetServiceTokenLastFetchedMissing.get();
}

const GetServiceTokenTestCase* GetServiceTokenLastFetchedInvalid() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenLastFetchedInvalid({
          .test_name = "get_service_token_cache_last_fetched_invalid",
          .service_tokens_dict = base::BindOnce([](base::Time) {
            return base::DictValue().Set(
                "email-aliases",
                base::DictValue()
                    .Set(prefs::keys::kServiceToken,
                         base::Base64Encode("cached_service_token"))
                    .Set(prefs::keys::kLastFetched, "invalid-time-format"));
          }),
          .fail_decryption = false,
          .fail_encryption = false,
          .time_advance = {},  // not used
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token =
                                           "fetched_service_token";
                                       return body;
                                     }()}},
          .mojo_expected =
              mojom::GetServiceTokenResult::New("fetched_service_token"),
      });
  return kGetServiceTokenLastFetchedInvalid.get();
}

const GetServiceTokenTestCase* GetServiceTokenCacheExpired() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenCacheExpired({
          .test_name = "get_service_token_cache_expired",
          .service_tokens_dict = base::BindOnce([](base::Time mock_now) {
            return base::DictValue().Set(
                "email-aliases",
                base::DictValue()
                    .Set(prefs::keys::kServiceToken,
                         base::Base64Encode("cached_service_token"))
                    .Set(prefs::keys::kLastFetched,
                         base::TimeToValue(mock_now)));
          }),
          .fail_decryption = false,
          .fail_encryption = false,
          // Advance time after setting cache to make it expired.
          .time_advance = kServiceTokenMaxAge + base::Minutes(1),
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token =
                                           "fetched_service_token";
                                       return body;
                                     }()}},
          .mojo_expected =
              mojom::GetServiceTokenResult::New("fetched_service_token"),
      });
  return kGetServiceTokenCacheExpired.get();
}

const GetServiceTokenTestCase* GetServiceTokenServiceTokenDecryptionFailed() {
  static const base::NoDestructor<GetServiceTokenTestCase>
      kGetServiceTokenServiceTokenDecryptionFailed({
          .test_name =
              "get_service_token_cache_service_token_decryption_failed",
          .service_tokens_dict = base::BindOnce([](base::Time mock_now) {
            return base::DictValue().Set(
                "email-aliases",
                base::DictValue()
                    .Set(prefs::keys::kServiceToken, "!!!invalid-base64!!!")
                    .Set(prefs::keys::kLastFetched,
                         base::TimeToValue(mock_now)));
          }),
          .fail_decryption = false,
          .fail_encryption = false,
          .time_advance = base::TimeDelta(),
          .endpoint_response = {{.net_error = net::OK,
                                 .status_code = net::HTTP_OK,
                                 .body =
                                     [] {
                                       ServiceToken::Response::SuccessBody body;
                                       body.auth_token =
                                           "fetched_service_token";
                                       return body;
                                     }()}},
          .mojo_expected =
              mojom::GetServiceTokenResult::New("fetched_service_token"),
      });
  return kGetServiceTokenServiceTokenDecryptionFailed.get();
}

using BraveAccountServiceGetServiceTokenTest =
    BraveAccountServiceTest<GetServiceTokenTestCase>;

}  // namespace

TEST_P(BraveAccountServiceGetServiceTokenTest, GetServiceToken) {
  RunTestCase();
}

INSTANTIATE_TEST_SUITE_P(
    BraveAccountServiceTests,
    BraveAccountServiceGetServiceTokenTest,
    testing::Values(GetServiceTokenCacheHit(),
                    GetServiceTokenAuthenticationTokenDecryptionFailed(),
                    GetServiceTokenNetworkError(),
                    GetServiceTokenBodyMissingOrFailedToParse(),
                    GetServiceTokenErrorCodeIsNull(),
                    GetServiceTokenEmailDomainNotSupported(),
                    GetServiceTokenIncorrectCredentials(),
                    GetServiceTokenInvalidTokenAudience(),
                    GetServiceTokenBadRequest(),
                    GetServiceTokenUnauthorized(),
                    GetServiceTokenServerError(),
                    GetServiceTokenUnknown(),
                    GetServiceTokenServiceTokenEmpty(),
                    GetServiceTokenServiceTokenEncryptionFailed(),
                    GetServiceTokenSuccess(),
                    GetServiceTokenServiceDictMissing(),
                    GetServiceTokenServiceTokenMissing(),
                    GetServiceTokenLastFetchedMissing(),
                    GetServiceTokenLastFetchedInvalid(),
                    GetServiceTokenCacheExpired(),
                    GetServiceTokenServiceTokenDecryptionFailed()),
    BraveAccountServiceGetServiceTokenTest::kNameGenerator);

}  // namespace brave_account
