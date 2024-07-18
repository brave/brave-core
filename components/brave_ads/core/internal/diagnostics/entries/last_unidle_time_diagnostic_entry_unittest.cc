/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/last_unidle_time_diagnostic_entry.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_entry_types.h"

// npm run test -- brave_unit_tests --filter=BraveAds.*

namespace brave_ads {

class BraveAdsLastUnIdleTimeDiagnosticEntryTest : public test::TestBase {};

TEST_F(BraveAdsLastUnIdleTimeDiagnosticEntryTest, LastUnIdleTime) {
  // Arrange
  AdvanceClockTo(test::TimeFromString("Mon, 8 July 1996 12:34:56"));

  LastUnIdleTimeDiagnosticEntry diagnostic_entry(test::Now());

  // Act & Assert
  EXPECT_EQ(DiagnosticEntryType::kLastUnIdleTime, diagnostic_entry.GetType());
  EXPECT_EQ("Last unidle time", diagnostic_entry.GetName());
  EXPECT_EQ("Monday, July 8, 1996 at 12:34:56\u202fPM",
            diagnostic_entry.GetValue());
}

TEST_F(BraveAdsLastUnIdleTimeDiagnosticEntryTest, WasNeverUnIdle) {
  // Arrange
  const LastUnIdleTimeDiagnosticEntry diagnostic_entry;

  // Act & Assert
  EXPECT_EQ(DiagnosticEntryType::kLastUnIdleTime, diagnostic_entry.GetType());
  EXPECT_EQ("Last unidle time", diagnostic_entry.GetName());
  EXPECT_EQ("Never", diagnostic_entry.GetValue());
}

}  // namespace brave_ads
