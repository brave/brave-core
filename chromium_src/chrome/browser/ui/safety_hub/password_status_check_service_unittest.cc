/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "src/chrome/browser/ui/safety_hub/password_status_check_service_unittest.cc"

TEST_P(PasswordStatusCheckServiceParameterizedCardTest,
       PasswordCardDataIsMarkedSafe) {
  // Based on test parameters, add different credential issues to the store.
  if (include_weak()) {
    profile_store().AddLogin(WeakForm());
  }
  if (include_compromised()) {
    profile_store().AddLogin(LeakedForm());
  }
  if (include_reused()) {
    profile_store().AddLogin(ReusedForm1());
    profile_store().AddLogin(ReusedForm2());
  }

  // The password card data should always be marked safe
  base::Value::Dict dict;
  dict.Set(safety_hub::kCardStateKey,
           static_cast<int>(safety_hub::SafetyHubCardState::kSafe));
  EXPECT_EQ(dict, service()->GetPasswordCardData(signed_in()));
}
