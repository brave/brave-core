/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/database/database_manager.h"

#include "base/scoped_observation.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/database/database_manager_observer.h"
#include "brave/components/brave_ads/core/internal/database/test/database_manager_observer_mock.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

// npm run test -- brave_unit_tests --filter=BraveAds*

namespace brave_ads {

class BraveAdsDatabaseManagerTest : public test::TestBase {
 protected:
  void SetUpMocks() override {
    // Register before `CreateOrOpen` fires in `MockDefaultAdsServiceState` so
    // the observer catches the initial database creation notifications.
    observation_.Observe(&GlobalState::GetInstance()->GetDatabaseManager());

    EXPECT_CALL(observer_mock_, OnWillCreateOrOpenDatabase);
    EXPECT_CALL(observer_mock_, OnDidCreateDatabase);
    EXPECT_CALL(observer_mock_, OnDatabaseIsReady);
    EXPECT_CALL(observer_mock_, OnDidOpenDatabase).Times(0);
    EXPECT_CALL(observer_mock_, OnFailedToCreateOrOpenDatabase).Times(0);
  }

  ::testing::StrictMock<DatabaseManagerObserverMock> observer_mock_;
  base::ScopedObservation<DatabaseManager, DatabaseManagerObserver>
      observation_{&observer_mock_};
};

TEST_F(BraveAdsDatabaseManagerTest, NotifiesObserversWhenDatabaseIsCreated) {
  // The observer expectations are set in `SetUpMocks` and verified on mock
  // destruction. Nothing further to assert.
}

}  // namespace brave_ads
