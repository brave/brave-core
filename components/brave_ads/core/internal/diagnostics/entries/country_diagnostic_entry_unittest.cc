/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/country_diagnostic_entry.h"

#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_entry_types.h"

// npm run test -- brave_unit_tests --filter=BraveAds.*

namespace brave_ads {

class BraveAdsCountryDiagnosticEntryTest : public test::TestBase {};

TEST_F(BraveAdsCountryDiagnosticEntryTest, GetValue) {
  // Arrange
  fake_locale_.SetCountryCode("KY");

  const CountryDiagnosticEntry diagnostic_entry;

  // Act & Assert
  EXPECT_EQ(DiagnosticEntryType::kCountry, diagnostic_entry.GetType());
  EXPECT_EQ("Country", diagnostic_entry.GetName());
  EXPECT_EQ("KY", diagnostic_entry.GetValue());
}

}  // namespace brave_ads
