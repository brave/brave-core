/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/entries/catalog_last_updated_diagnostic_entry.h"

#include "bat/ads/internal/base/unittest/unittest_base.h"
#include "bat/ads/internal/base/unittest/unittest_time_util.h"
#include "bat/ads/internal/catalog/catalog_util.h"
#include "bat/ads/internal/diagnostics/diagnostic_entry_types.h"

// npm run test -- brave_unit_tests --filter=BatAds.*

namespace ads {

class BatAdsCatalogLastUpdatedDiagnosticEntryTest : public UnitTestBase {
 protected:
  BatAdsCatalogLastUpdatedDiagnosticEntryTest() = default;

  ~BatAdsCatalogLastUpdatedDiagnosticEntryTest() override = default;
};

TEST_F(BatAdsCatalogLastUpdatedDiagnosticEntryTest, CatalogLastUpdated) {
  // Arrange
  AdvanceClockTo(
      TimeFromString("Wed, 18 Nov 1970 12:34:56", /*is_local*/ true));

  SetCatalogLastUpdated(Now());

  CatalogLastUpdatedDiagnosticEntry diagnostic_entry;

  // Act

  // Assert
  EXPECT_EQ(DiagnosticEntryType::kCatalogLastUpdated,
            diagnostic_entry.GetType());
  EXPECT_EQ("Catalog last updated", diagnostic_entry.GetName());
  EXPECT_EQ("Wednesday, November 18, 1970 at 12:34:56 PM",
            diagnostic_entry.GetValue());
}

TEST_F(BatAdsCatalogLastUpdatedDiagnosticEntryTest, CatalogNeverUpdated) {
  // Arrange
  CatalogLastUpdatedDiagnosticEntry diagnostic_entry;

  // Act

  // Assert
  EXPECT_EQ(DiagnosticEntryType::kCatalogLastUpdated,
            diagnostic_entry.GetType());
  EXPECT_EQ("Catalog last updated", diagnostic_entry.GetName());
  EXPECT_EQ("", diagnostic_entry.GetValue());
}

}  // namespace ads
