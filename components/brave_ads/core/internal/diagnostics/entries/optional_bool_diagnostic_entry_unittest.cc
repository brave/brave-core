/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/optional_bool_diagnostic_entry.h"

#include <optional>

#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_entry_types.h"

// npm run test -- brave_unit_tests --filter=BraveAds.*

namespace brave_ads {

class BraveAdsOptionalBoolDiagnosticEntryTest : public test::TestBase {};

TEST_F(BraveAdsOptionalBoolDiagnosticEntryTest, True) {
  // Arrange
  const OptionalBoolDiagnosticEntry diagnostic_entry(
      DiagnosticEntryType::kWalletValid, "Wallet valid",
      base::BindRepeating([]() -> std::optional<bool> { return true; }));

  // Act & Assert
  EXPECT_EQ(DiagnosticEntryType::kWalletValid, diagnostic_entry.GetType());
  EXPECT_EQ("Wallet valid", diagnostic_entry.GetName());
  EXPECT_EQ("true", diagnostic_entry.GetValue());
}

TEST_F(BraveAdsOptionalBoolDiagnosticEntryTest, False) {
  // Arrange
  const OptionalBoolDiagnosticEntry diagnostic_entry(
      DiagnosticEntryType::kWalletValid, "Wallet valid",
      base::BindRepeating([]() -> std::optional<bool> { return false; }));

  // Act & Assert
  EXPECT_EQ("false", diagnostic_entry.GetValue());
}

TEST_F(BraveAdsOptionalBoolDiagnosticEntryTest, NotApplicable) {
  // Arrange
  const OptionalBoolDiagnosticEntry diagnostic_entry(
      DiagnosticEntryType::kWalletValid, "Wallet valid",
      base::BindRepeating(
          []() -> std::optional<bool> { return std::nullopt; }));

  // Act & Assert
  EXPECT_EQ("N/A", diagnostic_entry.GetValue());
}

}  // namespace brave_ads
