/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/os_crypt/dpapi_risk.h"

#include <stdint.h>

#include "testing/gtest/include/gtest/gtest.h"

namespace brave {

namespace {

uint32_t AsBit(DPAPIRiskSignal signal) {
  return static_cast<uint32_t>(signal);
}

}  // namespace

TEST(DPAPIRiskTest, AnUnremarkableMachineHasNoSignals) {
  EXPECT_EQ(0u, ComputeDPAPIRiskSignals(DPAPIEnvironment()));
}

TEST(DPAPIRiskTest, EachConditionSetsOnlyItsOwnBit) {
  {
    DPAPIEnvironment environment;
    environment.domain_joined = true;
    EXPECT_EQ(AsBit(DPAPIRiskSignal::kDomainJoined),
              ComputeDPAPIRiskSignals(environment));
  }
  {
    DPAPIEnvironment environment;
    environment.azure_ad_joined = true;
    EXPECT_EQ(AsBit(DPAPIRiskSignal::kAzureADJoined),
              ComputeDPAPIRiskSignals(environment));
  }
  {
    DPAPIEnvironment environment;
    environment.device_managed = true;
    EXPECT_EQ(AsBit(DPAPIRiskSignal::kDeviceManaged),
              ComputeDPAPIRiskSignals(environment));
  }
  {
    DPAPIEnvironment environment;
    environment.user_data_dir_on_network = true;
    EXPECT_EQ(AsBit(DPAPIRiskSignal::kUserDataDirOnNetwork),
              ComputeDPAPIRiskSignals(environment));
  }
  {
    DPAPIEnvironment environment;
    environment.roaming_windows_profile = true;
    EXPECT_EQ(AsBit(DPAPIRiskSignal::kRoamingWindowsProfile),
              ComputeDPAPIRiskSignals(environment));
  }
  {
    DPAPIEnvironment environment;
    environment.fslogix_present = true;
    EXPECT_EQ(AsBit(DPAPIRiskSignal::kFSLogix),
              ComputeDPAPIRiskSignals(environment));
  }
}

TEST(DPAPIRiskTest, ConditionsCombine) {
  DPAPIEnvironment environment;
  environment.domain_joined = true;
  environment.user_data_dir_on_network = true;
  environment.fslogix_present = true;

  EXPECT_EQ(AsBit(DPAPIRiskSignal::kDomainJoined) |
                AsBit(DPAPIRiskSignal::kUserDataDirOnNetwork) |
                AsBit(DPAPIRiskSignal::kFSLogix),
            ComputeDPAPIRiskSignals(environment));
}

// The bits are written to `Local State` and read back by a later launch, so
// their values are part of the stored format and cannot be renumbered.
TEST(DPAPIRiskTest, BitValuesAreStable) {
  EXPECT_EQ(1u, AsBit(DPAPIRiskSignal::kDomainJoined));
  EXPECT_EQ(2u, AsBit(DPAPIRiskSignal::kAzureADJoined));
  EXPECT_EQ(4u, AsBit(DPAPIRiskSignal::kDeviceManaged));
  EXPECT_EQ(8u, AsBit(DPAPIRiskSignal::kUserDataDirOnNetwork));
  EXPECT_EQ(16u, AsBit(DPAPIRiskSignal::kRoamingWindowsProfile));
  EXPECT_EQ(32u, AsBit(DPAPIRiskSignal::kFSLogix));
}

}  // namespace brave
