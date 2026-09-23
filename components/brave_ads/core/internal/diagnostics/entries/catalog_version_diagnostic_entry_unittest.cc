/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/catalog_version_diagnostic_entry.h"

#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_entry_types.h"

// npm run test -- brave_unit_tests --filter=BraveAds.*

namespace brave_ads {

class BraveAdsCatalogVersionDiagnosticEntryTest : public test::TestBase {};

TEST_F(BraveAdsCatalogVersionDiagnosticEntryTest, GetValueForNoCatalogVersion) {
  // Arrange
  const CatalogVersionDiagnosticEntry diagnostic_entry;

  // Act & Assert
  EXPECT_EQ(DiagnosticEntryType::kCatalogVersion, diagnostic_entry.GetType());
  EXPECT_EQ("Catalog Version", diagnostic_entry.GetName());
  EXPECT_EQ("N/A", diagnostic_entry.GetValue());
}

TEST_F(BraveAdsCatalogVersionDiagnosticEntryTest, GetValueForCatalogVersion) {
  // Arrange
  SetCatalogVersion(/*version=*/1);

  const CatalogVersionDiagnosticEntry diagnostic_entry;

  // Act & Assert
  EXPECT_EQ("1", diagnostic_entry.GetValue());
}

}  // namespace brave_ads
