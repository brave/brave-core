// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_component_filters_provider.h"

#include <string>

#include "base/files/file_util.h"
#include "base/files/scoped_temp_dir.h"
#include "base/test/run_until.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider_manager.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace {

class HashObserver : public brave_shields::AdBlockFiltersProvider::Observer {
 public:
  void OnChanged(bool is_default_engine) override { notified_ = true; }

  bool notified() const { return notified_; }
  void reset() { notified_ = false; }

 private:
  bool notified_ = false;
};

}  // namespace

class AdBlockComponentFiltersProviderTest : public testing::Test {
 public:
  void SetUp() override {
    ASSERT_TRUE(temp_dir_.CreateUniqueTempDir());
    ASSERT_TRUE(temp_dir2_.CreateUniqueTempDir());
  }

  static void SimulateComponentReady(
      brave_shields::AdBlockComponentFiltersProvider& provider,
      const base::FilePath& path) {
    provider.OnComponentReady(path);
  }

 protected:
  base::test::TaskEnvironment task_environment_;
  base::ScopedTempDir temp_dir_;
  base::ScopedTempDir temp_dir2_;
};

TEST_F(AdBlockComponentFiltersProviderTest,
       DifferentComponentsHaveSeparateHashes) {
  brave_shields::AdBlockFiltersProviderManager manager;

  brave_shields::AdBlockComponentFiltersProvider provider_a(
      nullptr, &manager, "component_a", "", "Component A", 0,
      /*is_default_engine=*/true);
  brave_shields::AdBlockComponentFiltersProvider provider_b(
      nullptr, &manager, "component_b", "", "Component B", 0,
      /*is_default_engine=*/false);

  // Before OnComponentReady, hash should be nullopt.
  EXPECT_FALSE(provider_a.GetCacheKey().has_value());
  EXPECT_FALSE(provider_b.GetCacheKey().has_value());

  // Each component gets a different path, so hashes differ.
  SimulateComponentReady(provider_a, temp_dir_.GetPath());
  SimulateComponentReady(provider_b, temp_dir2_.GetPath());

  ASSERT_TRUE(provider_a.GetCacheKey().has_value());
  ASSERT_TRUE(provider_b.GetCacheKey().has_value());
  EXPECT_NE(provider_a.GetCacheKey().value(), provider_b.GetCacheKey().value());
}

TEST_F(AdBlockComponentFiltersProviderTest, ComponentUpdateChangesHash) {
  brave_shields::AdBlockFiltersProviderManager manager;

  brave_shields::AdBlockComponentFiltersProvider provider(
      nullptr, &manager, "component_a", "", "Component A", 0,
      /*is_default_engine=*/true);

  SimulateComponentReady(provider, temp_dir_.GetPath());
  std::string original_hash = provider.GetCacheKey().value();
  ASSERT_FALSE(original_hash.empty());

  // A component update with a new path (new version) changes the hash.
  SimulateComponentReady(provider, temp_dir2_.GetPath());
  ASSERT_TRUE(provider.GetCacheKey().has_value());
  EXPECT_NE(provider.GetCacheKey().value(), original_hash);
}
