// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/brave_shields/core/browser/ad_block_component_filters_provider.h"

#include "base/files/scoped_temp_dir.h"
#include "base/test/task_environment.h"
#include "brave/components/brave_shields/core/browser/ad_block_filters_provider.h"
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
