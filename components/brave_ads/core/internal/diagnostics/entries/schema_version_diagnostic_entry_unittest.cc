/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/schema_version_diagnostic_entry.h"

#include "base/strings/string_number_conversions.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_entry_types.h"
#include "brave/components/brave_ads/core/internal/legacy_migration/database/database_constants.h"

// npm run test -- brave_unit_tests --filter=BraveAds.*

namespace brave_ads {

class BraveAdsSchemaVersionDiagnosticEntryTest : public test::TestBase {};

TEST_F(BraveAdsSchemaVersionDiagnosticEntryTest, GetValue) {
  // Arrange
  const SchemaVersionDiagnosticEntry diagnostic_entry;

  // Act & Assert
  EXPECT_EQ(DiagnosticEntryType::kSchemaVersion, diagnostic_entry.GetType());
  EXPECT_EQ("Schema version", diagnostic_entry.GetName());
  EXPECT_EQ(base::NumberToString(database::kVersionNumber),
            diagnostic_entry.GetValue());
}

}  // namespace brave_ads
