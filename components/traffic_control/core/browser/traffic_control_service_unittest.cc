// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/traffic_control/core/browser/traffic_control_service.h"

#include <memory>

#include "base/test/scoped_feature_list.h"
#include "brave/components/traffic_control/core/browser/pref_names.h"
#include "brave/components/traffic_control/core/browser/prefs_registration.h"
#include "brave/components/traffic_control/core/common/features.h"
#include "components/sync_preferences/testing_pref_service_syncable.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace traffic_control {

class TrafficControlServiceTest : public testing::Test {
 public:
  TrafficControlServiceTest() {
    feature_list_.InitAndEnableFeature(features::kTrafficControl);
    RegisterProfilePrefs(prefs_.registry());
    service_ = std::make_unique<TrafficControlService>(&prefs_);
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  sync_preferences::TestingPrefServiceSyncable prefs_;
  std::unique_ptr<TrafficControlService> service_;
};

TEST_F(TrafficControlServiceTest, EnabledPref) {
  EXPECT_FALSE(service_->IsEnabled());
  prefs_.SetBoolean(prefs::kTrafficControlEnabled, true);
  EXPECT_TRUE(service_->IsEnabled());
}

}  // namespace traffic_control
