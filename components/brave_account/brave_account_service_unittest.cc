/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_service.h"

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "base/test/task_environment.h"
#include "base/time/time.h"
#include "base/types/expected.h"
#include "base/values.h"
#include "brave/components/brave_account/brave_account_service_constants.h"
#include "brave/components/brave_account/brave_account_service_test.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoints/auth_validate.h"
#include "brave/components/brave_account/mock_brave_account_authentication_observer.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "components/prefs/pref_service.h"
#include "net/http/http_status_code.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

using endpoints::AuthValidate;

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

}  // namespace brave_account
