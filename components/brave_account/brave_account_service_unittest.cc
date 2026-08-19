/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_service.h"

#include <string>

#include "base/no_destructor.h"
#include "brave/components/brave_account/brave_account_service_test.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/mock_brave_account_authentication_observer.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "components/prefs/pref_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

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

}  // namespace brave_account
