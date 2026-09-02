/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/sponsored_ads_enabled_diagnostic_entry.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_entry_types.h"
#include "brave/components/brave_ads/core/internal/settings/test/settings_test_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds.*

namespace brave_ads {

class BraveAdsSponsoredAdsEnabledDiagnosticEntryTest : public test::TestBase {};

TEST_F(BraveAdsSponsoredAdsEnabledDiagnosticEntryTest, IsEnabled) {
  // Arrange
  const SponsoredAdsEnabledDiagnosticEntry diagnostic_entry;

  // Act & Assert
  EXPECT_EQ(DiagnosticEntryType::kSponsoredAdsEnabled,
            diagnostic_entry.GetType());
  EXPECT_EQ("Sponsored ads enabled", diagnostic_entry.GetName());
  EXPECT_EQ("true", diagnostic_entry.GetValue());
}

TEST_F(BraveAdsSponsoredAdsEnabledDiagnosticEntryTest, IsDisabled) {
  // Arrange
  test::DisableSponsoredAds();

  const SponsoredAdsEnabledDiagnosticEntry diagnostic_entry;

  // Act & Assert
  EXPECT_EQ(DiagnosticEntryType::kSponsoredAdsEnabled,
            diagnostic_entry.GetType());
  EXPECT_EQ("Sponsored ads enabled", diagnostic_entry.GetName());
  EXPECT_EQ("false", diagnostic_entry.GetValue());
}

}  // namespace brave_ads
