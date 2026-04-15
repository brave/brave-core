// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_dat_cache_manager.h"

#include "base/files/scoped_temp_dir.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "components/prefs/testing_pref_service.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace brave_shields {

class AdBlockDATCacheManagerTest : public testing::Test {
 public:
  AdBlockDATCacheManagerTest() {
    feature_list_.InitAndEnableFeature(features::kAdblockDATCache);
  }

  void SetUp() override {
    ASSERT_TRUE(profile_dir_.CreateUniqueTempDir());
    AdBlockDATCacheManager::RegisterPrefs(prefs_.registry());
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir profile_dir_;
  TestingPrefServiceSimple prefs_;
};

}  // namespace brave_shields
