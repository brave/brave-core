/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/os_crypt/storage_durability.h"

#include <stdint.h>

#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

namespace {

uint32_t AsBit(StorageDurabilitySignal signal) {
  return static_cast<uint32_t>(signal);
}

// A drive that answered, caches writes, and flushes them when asked.
StorageDurability HealthyDrive() {
  StorageDurability durability;
  durability.queried = true;
  durability.write_cache_enabled = true;
  durability.flush_cache_supported = true;
  return durability;
}

}  // namespace

TEST(StorageDurabilityTest, ADriveThatCannotBeAskedReportsOnlyThat) {
  StorageDurability durability;
  durability.queried = false;
  // Anything else set is meaningless and must not leak into the signals.
  durability.write_cache_enabled = true;
  durability.user_defined_power_protection = true;

  EXPECT_EQ(AsBit(StorageDurabilitySignal::kQueryFailed),
            ComputeStorageDurabilitySignals(durability));
}

TEST(StorageDurabilityTest, AHealthyDriveReportsOnlyItsCache) {
  EXPECT_EQ(AsBit(StorageDurabilitySignal::kWriteCacheEnabled),
            ComputeStorageDurabilitySignals(HealthyDrive()));
}

TEST(StorageDurabilityTest, SignalsAreReportedIndependently) {
  StorageDurability durability = HealthyDrive();
  durability.user_defined_power_protection = true;
  durability.nv_cache_enabled = true;
  durability.flush_cache_supported = false;

  EXPECT_EQ(AsBit(StorageDurabilitySignal::kWriteCacheEnabled) |
                AsBit(StorageDurabilitySignal::kUserDefinedPowerProtection) |
                AsBit(StorageDurabilitySignal::kNVCacheEnabled) |
                AsBit(StorageDurabilitySignal::kFlushNotSupported),
            ComputeStorageDurabilitySignals(durability));
}

// The combination this whole file exists to find: Windows has been told to stop
// flushing, the drive is caching, and nothing is backing the cache up.
TEST(StorageDurabilityTest, FlushingDisabledWithoutBatteryBackingIsUnsafe) {
  StorageDurability durability = HealthyDrive();
  durability.user_defined_power_protection = true;

  EXPECT_TRUE(
      IsWriteCacheFlushingUnsafe(ComputeStorageDurabilitySignals(durability)));
}

// The same setting is the correct one on hardware with a battery-backed cache,
// so it must not be reported as a risk there.
TEST(StorageDurabilityTest, BatteryBackedCacheIsNotUnsafe) {
  StorageDurability durability = HealthyDrive();
  durability.user_defined_power_protection = true;
  durability.nv_cache_enabled = true;

  EXPECT_FALSE(
      IsWriteCacheFlushingUnsafe(ComputeStorageDurabilitySignals(durability)));
}

TEST(StorageDurabilityTest, CachingAloneIsNotUnsafe) {
  EXPECT_FALSE(IsWriteCacheFlushingUnsafe(
      ComputeStorageDurabilitySignals(HealthyDrive())));
}

TEST(StorageDurabilityTest, ADriveThatCannotBeAskedIsNotReportedAsUnsafe) {
  EXPECT_FALSE(IsWriteCacheFlushingUnsafe(
      ComputeStorageDurabilitySignals(StorageDurability())));
}

// These bits are written to `Local State` and read back by a later launch, so
// their values are part of the stored format.
TEST(StorageDurabilityTest, BitValuesAreStable) {
  EXPECT_EQ(1u, AsBit(StorageDurabilitySignal::kQueryFailed));
  EXPECT_EQ(2u, AsBit(StorageDurabilitySignal::kWriteCacheEnabled));
  EXPECT_EQ(4u, AsBit(StorageDurabilitySignal::kUserDefinedPowerProtection));
  EXPECT_EQ(8u, AsBit(StorageDurabilitySignal::kNVCacheEnabled));
  EXPECT_EQ(16u, AsBit(StorageDurabilitySignal::kFlushNotSupported));
}

}  // namespace brave
