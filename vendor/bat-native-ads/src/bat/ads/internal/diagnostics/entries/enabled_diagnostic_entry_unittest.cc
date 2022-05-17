/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/entries/enabled_diagnostic_entry.h"

#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/diagnostics/diagnostic_entry_types.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds.*

namespace ads {

class BatAdsEnabledDiagnosticEntryTest : public UnitTestBase {
 protected:
  BatAdsEnabledDiagnosticEntryTest() = default;

  ~BatAdsEnabledDiagnosticEntryTest() override = default;
};

TEST_F(BatAdsEnabledDiagnosticEntryTest, Enabled) {
  // Arrange
  AdsClientHelper::Get()->SetBooleanPref(prefs::kEnabled, true);

  EnabledDiagnosticEntry diagnostic_entry;

  // Act

  // Assert
  EXPECT_EQ(DiagnosticEntryType::kEnabled, diagnostic_entry.GetType());
  EXPECT_EQ("Enabled", diagnostic_entry.GetName());
  EXPECT_EQ("true", diagnostic_entry.GetValue());
}

TEST_F(BatAdsEnabledDiagnosticEntryTest, Disabled) {
  // Arrange
  AdsClientHelper::Get()->SetBooleanPref(prefs::kEnabled, false);

  EnabledDiagnosticEntry diagnostic_entry;

  // Act

  // Assert
  EXPECT_EQ(DiagnosticEntryType::kEnabled, diagnostic_entry.GetType());
  EXPECT_EQ("Enabled", diagnostic_entry.GetName());
  EXPECT_EQ("false", diagnostic_entry.GetValue());
}

}  // namespace ads
