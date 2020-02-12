/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/password_manager/core/browser/password_bubble_experiment.h"

#include <ostream>

#include "components/password_manager/core/common/password_manager_pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/testing_pref_service.h"
#include "components/sync/driver/test_sync_service.h"
#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace password_bubble_experiment {

class PasswordManagerPasswordBubbleExperimentTest : public testing::Test {
 public:
  PasswordManagerPasswordBubbleExperimentTest() {
    RegisterPrefs(pref_service_.registry());
  }

  PrefService* prefs() { return &pref_service_; }

  syncer::TestSyncService* sync_service() { return &fake_sync_service_; }

 private:
  syncer::TestSyncService fake_sync_service_;
  TestingPrefServiceSimple pref_service_;
};

TEST_F(PasswordManagerPasswordBubbleExperimentTest,
       ShouldShowChromeSignInPasswordPromo) {
  // By default the promo is off.
  EXPECT_FALSE(ShouldShowChromeSignInPasswordPromo(prefs(), nullptr));
  constexpr struct {
    bool was_already_clicked;
    bool is_sync_allowed;
    bool is_first_setup_complete;
    int current_shown_count;
    bool result;
  } kTestData[] = {
      {false, true, false, 0, false},   {false, true, false, 5, false},
      {true, true, false, 0, false},   {true, true, false, 10, false},
      {false, false, false, 0, false}, {false, true, true, 0, false},
  };
  for (const auto& test_case : kTestData) {
    SCOPED_TRACE(testing::Message("#test_case = ") << (&test_case - kTestData));
    prefs()->SetBoolean(password_manager::prefs::kWasSignInPasswordPromoClicked,
                        test_case.was_already_clicked);
    prefs()->SetInteger(
        password_manager::prefs::kNumberSignInPasswordPromoShown,
        test_case.current_shown_count);
    sync_service()->SetDisableReasons(
        test_case.is_sync_allowed
            ? syncer::SyncService::DisableReasonSet()
            : syncer::SyncService::DISABLE_REASON_ENTERPRISE_POLICY);
    sync_service()->SetFirstSetupComplete(test_case.is_first_setup_complete);
    sync_service()->SetTransportState(
        test_case.is_first_setup_complete
            ? syncer::SyncService::TransportState::ACTIVE
            : syncer::SyncService::TransportState::
                  PENDING_DESIRED_CONFIGURATION);

    EXPECT_EQ(test_case.result,
              ShouldShowChromeSignInPasswordPromo(prefs(), sync_service()));
  }
}

}  // namespace password_bubble_experiment
