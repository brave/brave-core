/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/entries/last_unidle_time_diagnostic_entry.h"

#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_time_util.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/diagnostics/diagnostic_entry_types.h"

// npm run test -- brave_unit_tests --filter=BatAds.*

namespace ads {

class BatAdsLastUnIdleTimeDiagnosticEntryTest : public UnitTestBase {
 protected:
  BatAdsLastUnIdleTimeDiagnosticEntryTest() = default;

  ~BatAdsLastUnIdleTimeDiagnosticEntryTest() override = default;
};

TEST_F(BatAdsLastUnIdleTimeDiagnosticEntryTest, LastUnIdleTime) {
  // Arrange
  AdvanceClock(
      TimeFromString("Mon, 8 June 1996 12:34:56", /* is_local */ false));

  LastUnIdleTimeDiagnosticEntry diagnostic_entry;

  // Act
  diagnostic_entry.SetLastUnIdleTime(Now());

  // Assert
  EXPECT_EQ(DiagnosticEntryType::kLastUnIdleTime, diagnostic_entry.GetType());
  EXPECT_EQ("Last unidle time", diagnostic_entry.GetName());
  EXPECT_EQ("1996-06-08T12:34:56.000Z", diagnostic_entry.GetValue());
}

TEST_F(BatAdsLastUnIdleTimeDiagnosticEntryTest, NeverUnIdleTime) {
  // Arrange
  LastUnIdleTimeDiagnosticEntry diagnostic_entry;

  // Act

  // Assert
  EXPECT_EQ(DiagnosticEntryType::kLastUnIdleTime, diagnostic_entry.GetType());
  EXPECT_EQ("Last unidle time", diagnostic_entry.GetName());
  EXPECT_EQ("", diagnostic_entry.GetValue());
}

}  // namespace ads
