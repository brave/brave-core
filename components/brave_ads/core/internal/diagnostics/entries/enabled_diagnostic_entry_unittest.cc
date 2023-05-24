/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/enabled_diagnostic_entry.h"

#include "brave/components/brave_ads/core/internal/ads/ad_unittest_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_entry_types.h"

// npm run test -- brave_unit_tests --filter=BraveAds.*

namespace brave_ads {

class BraveAdsEnabledDiagnosticEntryTest : public UnitTestBase {};

TEST_F(BraveAdsEnabledDiagnosticEntryTest, IsEnabled) {
  // Arrange

  // Act
  const EnabledDiagnosticEntry diagnostic_entry;

  // Assert
  EXPECT_EQ(DiagnosticEntryType::kEnabled, diagnostic_entry.GetType());
  EXPECT_EQ("Enabled", diagnostic_entry.GetName());
  EXPECT_EQ("true", diagnostic_entry.GetValue());
}

TEST_F(BraveAdsEnabledDiagnosticEntryTest, IsDisabled) {
  // Arrange
  DisableBravePrivateAds();

  // Act
  const EnabledDiagnosticEntry diagnostic_entry;

  // Assert
  EXPECT_EQ(DiagnosticEntryType::kEnabled, diagnostic_entry.GetType());
  EXPECT_EQ("Enabled", diagnostic_entry.GetName());
  EXPECT_EQ("false", diagnostic_entry.GetValue());
}

}  // namespace brave_ads
