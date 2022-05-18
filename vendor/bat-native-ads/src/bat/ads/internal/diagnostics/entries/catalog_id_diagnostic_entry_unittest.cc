/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/entries/catalog_id_diagnostic_entry.h"

#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/unittest_base.h"
#include "bat/ads/internal/base/unittest_util.h"
#include "bat/ads/internal/diagnostics/diagnostic_entry_types.h"
#include "bat/ads/pref_names.h"

// npm run test -- brave_unit_tests --filter=BatAds.*

namespace ads {

class BatAdsCatalogIdDiagnosticEntryTest : public UnitTestBase {
 protected:
  BatAdsCatalogIdDiagnosticEntryTest() = default;

  ~BatAdsCatalogIdDiagnosticEntryTest() override = default;
};

TEST_F(BatAdsCatalogIdDiagnosticEntryTest, CatalogId) {
  // Arrange
  AdsClientHelper::Get()->SetStringPref(prefs::kCatalogId,
                                        "da5dd0e8-71e9-4607-a45b-13e28b607a81");

  CatalogIdDiagnosticEntry diagnostic_entry;

  // Act

  // Assert
  EXPECT_EQ(DiagnosticEntryType::kCatalogId, diagnostic_entry.GetType());
  EXPECT_EQ("Catalog ID", diagnostic_entry.GetName());
  EXPECT_EQ("da5dd0e8-71e9-4607-a45b-13e28b607a81",
            diagnostic_entry.GetValue());
}

TEST_F(BatAdsCatalogIdDiagnosticEntryTest, EmptyCatalogId) {
  // Arrange
  CatalogIdDiagnosticEntry diagnostic_entry;

  // Act

  // Assert
  EXPECT_EQ(DiagnosticEntryType::kCatalogId, diagnostic_entry.GetType());
  EXPECT_EQ("Catalog ID", diagnostic_entry.GetName());
  EXPECT_EQ("", diagnostic_entry.GetValue());
}

}  // namespace ads
