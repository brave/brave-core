/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/entries/last_unidle_time_diagnostic_entry.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/common/unittest/unittest_time_util.h"
#include "bat/ads/internal/diagnostics/diagnostic_entry_types.h"

// npm run test -- brave_unit_tests --filter=BatAds.*

namespace ads {

class BatAdsLastUnIdleTimeDiagnosticEntryTest : public UnitTestBase {};

TEST_F(BatAdsLastUnIdleTimeDiagnosticEntryTest, LastUnIdleTime) {
  // Arrange
  AdvanceClockTo(
      TimeFromString("Mon, 8 July 1996 12:34:56", /*is_local*/ true));

  LastUnIdleTimeDiagnosticEntry diagnostic_entry;

  // Act
  diagnostic_entry.SetLastUnIdleTime(Now());

  // Assert
  EXPECT_EQ(DiagnosticEntryType::kLastUnIdleTime, diagnostic_entry.GetType());
  EXPECT_EQ("Last unidle time", diagnostic_entry.GetName());
  EXPECT_EQ("Monday, July 8, 1996 at 12:34:56\u202fPM",
            diagnostic_entry.GetValue());
}

TEST_F(BatAdsLastUnIdleTimeDiagnosticEntryTest, WasNeverUnIdle) {
  // Arrange
  const LastUnIdleTimeDiagnosticEntry diagnostic_entry;

  // Act

  // Assert
  EXPECT_EQ(DiagnosticEntryType::kLastUnIdleTime, diagnostic_entry.GetType());
  EXPECT_EQ("Last unidle time", diagnostic_entry.GetName());
  EXPECT_EQ("Never", diagnostic_entry.GetValue());
}

}  // namespace ads
