/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/flows/log_out.h"

#include <string>

#include "base/check_deref.h"
#include "base/no_destructor.h"
#include "brave/components/brave_account/brave_account_service_test.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "components/prefs/pref_service.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_account {

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
  static const base::NoDestructor<LogOutTestCase> kTestCase({
      .test_name = "log_out_authentication_token_non_empty",
  });
  return kTestCase.get();
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

}  // namespace brave_account
