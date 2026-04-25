/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/catalog_last_updated_diagnostic_entry.h"

#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/common/test/time_test_util.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_entry_types.h"

// npm run test -- brave_unit_tests --filter=BraveAds.*

namespace brave_ads {

class BraveAdsCatalogLastUpdatedDiagnosticEntryTest : public test::TestBase {};

TEST_F(BraveAdsCatalogLastUpdatedDiagnosticEntryTest, CatalogLastUpdated) {
  // Arrange
  AdvanceClockTo(test::TimeFromString("Wed, 18 Nov 1970 12:34:56"));

  SetCatalogLastUpdated(test::Now());

  const CatalogLastUpdatedDiagnosticEntry diagnostic_entry;

  // Act & Assert
  EXPECT_EQ(DiagnosticEntryType::kCatalogLastUpdated,
            diagnostic_entry.GetType());
  EXPECT_EQ("Catalog last updated", diagnostic_entry.GetName());
  EXPECT_EQ("Wednesday, November 18, 1970 at 12:34:56\u202fPM",
            diagnostic_entry.GetValue());
}

TEST_F(BraveAdsCatalogLastUpdatedDiagnosticEntryTest, CatalogNeverUpdated) {
  // Arrange
  const CatalogLastUpdatedDiagnosticEntry diagnostic_entry;

  // Act & Assert
  EXPECT_EQ(DiagnosticEntryType::kCatalogLastUpdated,
            diagnostic_entry.GetType());
  EXPECT_EQ("Catalog last updated", diagnostic_entry.GetName());
  EXPECT_THAT(diagnostic_entry.GetValue(), ::testing::IsEmpty());
}

}  // namespace brave_ads
