/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/opted_in_to_notification_ads_diagnostic_entry.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_entry_types.h"
#include "brave/components/brave_ads/core/internal/settings/settings_unittest_util.h"

// npm run test -- brave_unit_tests --filter=BraveAds.*

namespace brave_ads {

class BraveAdsOptedInToNotificationAdsDiagnosticEntryTest
    : public UnitTestBase {};

TEST_F(BraveAdsOptedInToNotificationAdsDiagnosticEntryTest, IsOptedIn) {
  // Arrange
  const OptedInToNotificationAdsDiagnosticEntry diagnostic_entry;

  // Act & Assert
  EXPECT_EQ(DiagnosticEntryType::kOptedInToNotificationAds,
            diagnostic_entry.GetType());
  EXPECT_EQ("Opted-in to notification ads", diagnostic_entry.GetName());
  EXPECT_EQ("true", diagnostic_entry.GetValue());
}

TEST_F(BraveAdsOptedInToNotificationAdsDiagnosticEntryTest, IsOptedOut) {
  // Arrange
  test::OptOutOfNotificationAds();

  const OptedInToNotificationAdsDiagnosticEntry diagnostic_entry;

  // Act & Assert
  EXPECT_EQ(DiagnosticEntryType::kOptedInToNotificationAds,
            diagnostic_entry.GetType());
  EXPECT_EQ("Opted-in to notification ads", diagnostic_entry.GetName());
  EXPECT_EQ("false", diagnostic_entry.GetValue());
}

}  // namespace brave_ads
