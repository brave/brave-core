// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_dat_cache_manager.h"

#include <optional>

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
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

// HasCachedDAT is per-engine and WriteDATFile sets the timestamp for
// that engine only.
TEST_F(AdBlockDATCacheManagerTest, HasCachedDATIsPerEngine) {
  AdBlockDATCacheManager cache(&prefs_, profile_dir_.GetPath());

  EXPECT_FALSE(cache.HasCachedDAT(true));
  EXPECT_FALSE(cache.HasCachedDAT(false));

  // Write only the default engine DAT.
  DATFileDataBuffer dat_data(100, 'x');
  base::test::TestFuture<bool> future;
  cache.WriteDATFile(true, std::move(dat_data), future.GetCallback());
  EXPECT_TRUE(future.Get());

  // Only the default engine should report a cached DAT.
  EXPECT_TRUE(cache.HasCachedDAT(true));
  EXPECT_FALSE(cache.HasCachedDAT(false));

  // Write the additional engine DAT.
  DATFileDataBuffer dat_data2(100, 'y');
  base::test::TestFuture<bool> future2;
  cache.WriteDATFile(false, std::move(dat_data2), future2.GetCallback());
  EXPECT_TRUE(future2.Get());

  EXPECT_TRUE(cache.HasCachedDAT(true));
  EXPECT_TRUE(cache.HasCachedDAT(false));
}

}  // namespace brave_shields
