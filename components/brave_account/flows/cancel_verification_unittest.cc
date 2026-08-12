/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/cancel_verification.h"

#include <string>

#include "base/no_destructor.h"
#include "brave/components/brave_account/brave_account_service_test.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

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
  static const base::NoDestructor<CancelVerificationTestCase> kTestCase({
      .test_name = "cancel_registration_verification_token_non_empty",
  });
  return kTestCase.get();
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
