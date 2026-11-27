/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/test/values_test_util.h"

#include <chrome/browser/ui/safety_hub/password_status_check_service_unittest.cc>

TEST_P(PasswordStatusCheckServiceParameterizedCardTest,
       PasswordCardDataIsMarkedSafe) {
  // Based on test parameters, add different credential issues to the store.
  if (include_weak()) {
    profile_store().AddLogin(password_manager::FromPasswordForm(WeakForm()));
  }
  if (include_compromised()) {
    profile_store().AddLogin(password_manager::FromPasswordForm(LeakedForm()));
  }
  if (include_reused()) {
    profile_store().AddLogin(password_manager::FromPasswordForm(ReusedForm1()));
    profile_store().AddLogin(password_manager::FromPasswordForm(ReusedForm2()));
  }

  // The password card is hidden in brave://settings/safetyCheck, so it always
  // reports the safe state (3 == SafetyHubCardState::kSafe).
  EXPECT_EQ(base::test::ParseJsonDict(R"({"state": 3})"),
            service()->GetPasswordCardData(signed_in()));
}
