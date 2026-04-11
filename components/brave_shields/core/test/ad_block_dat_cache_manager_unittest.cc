// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_dat_cache_manager.h"

#include <optional>
#include <string>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/scoped_feature_list.h"
#include "base/test/task_environment.h"
#include "base/test/test_future.h"
#include "brave/components/brave_shields/content/test/test_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"
#include "brave/components/brave_shields/core/common/features.h"
#include "brave/components/brave_shields/core/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
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
    prefs_.registry()->RegisterStringPref(prefs::kAdBlockDefaultCacheHash, "");
    prefs_.registry()->RegisterStringPref(prefs::kAdBlockAdditionalCacheHash,
                                          "");
  }

 protected:
  base::test::ScopedFeatureList feature_list_;
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir profile_dir_;
  TestingPrefServiceSimple prefs_;
  AdBlockFiltersProviderManager manager_;
};

TEST_F(AdBlockDATCacheManagerTest, ShouldLoadWhenNoCachedHash) {
  AdBlockDATCacheManager cache(&prefs_, &manager_, profile_dir_.GetPath());

  // No cached hash in prefs — should always load.
  EXPECT_TRUE(cache.ShouldLoadFilterSet(true));
  EXPECT_TRUE(cache.ShouldLoadFilterSet(false));
}

TEST_F(AdBlockDATCacheManagerTest, ShouldSkipWhenCacheKeyMatches) {
  TestFiltersProvider provider("||example.com^", true, 0);
  provider.RegisterAsSourceProvider(&manager_);

  AdBlockDATCacheManager cache(&prefs_, &manager_, profile_dir_.GetPath());

  // Store the cache key (simulates a previous successful build).
  cache.StoreCacheKey(true, cache.ComputeCombinedCacheKey(true));

  // Now the combined key matches the cached key — should skip loading.
  EXPECT_FALSE(cache.ShouldLoadFilterSet(true));
}

TEST_F(AdBlockDATCacheManagerTest, ShouldLoadWhenCacheKeyMismatches) {
  TestFiltersProvider provider("||example.com^", true, 0);
  provider.RegisterAsSourceProvider(&manager_);

  AdBlockDATCacheManager cache(&prefs_, &manager_, profile_dir_.GetPath());

  // Store a stale cache key that doesn't match.
  prefs_.SetString(prefs::kAdBlockDefaultCacheHash, "stale_key");

  // Combined key won't match — should load.
  EXPECT_TRUE(cache.ShouldLoadFilterSet(true));
}

TEST_F(AdBlockDATCacheManagerTest, StoreCacheKeyWritesToPrefs) {
  TestFiltersProvider provider("||example.com^", true, 0);
  provider.RegisterAsSourceProvider(&manager_);

  AdBlockDATCacheManager cache(&prefs_, &manager_, profile_dir_.GetPath());

  EXPECT_EQ(prefs_.GetString(prefs::kAdBlockDefaultCacheHash), "");

  cache.StoreCacheKey(true, cache.ComputeCombinedCacheKey(true));

  EXPECT_NE(prefs_.GetString(prefs::kAdBlockDefaultCacheHash), "");
  EXPECT_EQ(prefs_.GetString(prefs::kAdBlockDefaultCacheHash),
            cache.ComputeCombinedCacheKey(true));
}

TEST_F(AdBlockDATCacheManagerTest, ComputeCombinedCacheKeyExcludesNullopt) {
  // A provider that returns nullopt should be excluded from the combined key.
  TestFiltersProvider provider_a("||a.com^", true, 0);
  provider_a.RegisterAsSourceProvider(&manager_);

  TestFiltersProvider provider_b("||b.com^", true, 0);
  provider_b.set_cache_key_nullopt();
  provider_b.RegisterAsSourceProvider(&manager_);

  AdBlockDATCacheManager cache(&prefs_, &manager_, profile_dir_.GetPath());

  // Combined key should only include provider_a's key.
  std::string combined = cache.ComputeCombinedCacheKey(true);
  EXPECT_FALSE(combined.empty());

  // Should differ from the key when both providers have values.
  provider_b.set_cache_key("some_key");
  std::string combined_with_both = cache.ComputeCombinedCacheKey(true);
  EXPECT_NE(combined, combined_with_both);
}

TEST_F(AdBlockDATCacheManagerTest, WriteDATFileCreatesCacheFile) {
  AdBlockDATCacheManager cache(&prefs_, &manager_, profile_dir_.GetPath());

  DATFileDataBuffer dat_data(100, 'x');
  base::test::TestFuture<bool> future;

  cache.WriteDATFile(true, std::move(dat_data), future.GetCallback());

  EXPECT_TRUE(future.Get());
  EXPECT_TRUE(base::PathExists(
      profile_dir_.GetPath().AppendASCII("adblock_cache/engine0.dat")));
}

TEST_F(AdBlockDATCacheManagerTest, ReadCachedDATFilesReturnsData) {
  // Write DAT files manually.
  base::FilePath cache_dir =
      profile_dir_.GetPath().AppendASCII("adblock_cache");
  ASSERT_TRUE(base::CreateDirectory(cache_dir));
  ASSERT_TRUE(base::WriteFile(cache_dir.AppendASCII("engine0.dat"), "default"));
  ASSERT_TRUE(
      base::WriteFile(cache_dir.AppendASCII("engine1.dat"), "additional"));

  AdBlockDATCacheManager cache(&prefs_, &manager_, profile_dir_.GetPath());

  base::test::TestFuture<std::optional<DATFileDataBuffer>,
                         std::optional<DATFileDataBuffer>>
      future;
  cache.ReadCachedDATFiles(future.GetCallback());

  auto [default_dat, additional_dat] = future.Get();
  ASSERT_TRUE(default_dat.has_value());
  ASSERT_TRUE(additional_dat.has_value());
  EXPECT_EQ(std::string(default_dat->begin(), default_dat->end()), "default");
  EXPECT_EQ(std::string(additional_dat->begin(), additional_dat->end()),
            "additional");
}

TEST_F(AdBlockDATCacheManagerTest,
       ReadCachedDATFilesReturnsNulloptWhenMissing) {
  AdBlockDATCacheManager cache(&prefs_, &manager_, profile_dir_.GetPath());

  base::test::TestFuture<std::optional<DATFileDataBuffer>,
                         std::optional<DATFileDataBuffer>>
      future;
  cache.ReadCachedDATFiles(future.GetCallback());

  auto [default_dat, additional_dat] = future.Get();
  EXPECT_FALSE(default_dat.has_value());
  EXPECT_FALSE(additional_dat.has_value());
}

TEST_F(AdBlockDATCacheManagerTest, AllowDatLoadingFlag) {
  AdBlockDATCacheManager cache(&prefs_, &manager_, profile_dir_.GetPath());

  EXPECT_TRUE(cache.allow_dat_loading());
  cache.set_allow_dat_loading(false);
  EXPECT_FALSE(cache.allow_dat_loading());
}

}  // namespace brave_shields
