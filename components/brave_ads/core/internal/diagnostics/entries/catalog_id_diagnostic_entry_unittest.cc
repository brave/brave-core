/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/catalog_id_diagnostic_entry.h"

#include "brave/components/brave_ads/core/internal/catalog/catalog_test_constants.h"
#include "brave/components/brave_ads/core/internal/catalog/catalog_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_entry_types.h"

// npm run test -- brave_unit_tests --filter=BraveAds.*

namespace brave_ads {

class BraveAdsCatalogIdDiagnosticEntryTest : public test::TestBase {};

TEST_F(BraveAdsCatalogIdDiagnosticEntryTest, CatalogId) {
  // Arrange
  SetCatalogId(test::kCatalogId);

  const CatalogIdDiagnosticEntry diagnostic_entry;

  // Act & Assert
  EXPECT_EQ(DiagnosticEntryType::kCatalogId, diagnostic_entry.GetType());
  EXPECT_EQ("Catalog ID", diagnostic_entry.GetName());
  EXPECT_EQ(test::kCatalogId, diagnostic_entry.GetValue());
}

TEST_F(BraveAdsCatalogIdDiagnosticEntryTest, EmptyCatalogId) {
  // Arrange
  const CatalogIdDiagnosticEntry diagnostic_entry;

  // Act & Assert
  EXPECT_EQ(DiagnosticEntryType::kCatalogId, diagnostic_entry.GetType());
  EXPECT_EQ("Catalog ID", diagnostic_entry.GetName());
  EXPECT_THAT(diagnostic_entry.GetValue(), ::testing::IsEmpty());
}

}  // namespace brave_ads
