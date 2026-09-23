/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/confirmation_tokens_remaining_diagnostic_entry.h"

#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/test/confirmation_tokens_test_util.h"
#include "brave/components/brave_ads/core/internal/common/test/test_base.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_entry_types.h"

// npm run test -- brave_unit_tests --filter=BraveAds.*

namespace brave_ads {

class BraveAdsConfirmationTokensRemainingDiagnosticEntryTest
    : public test::TestBase {};

TEST_F(BraveAdsConfirmationTokensRemainingDiagnosticEntryTest, NoTokens) {
  // Arrange
  const ConfirmationTokensRemainingDiagnosticEntry diagnostic_entry;

  // Act & Assert
  EXPECT_EQ(DiagnosticEntryType::kConfirmationTokensRemaining,
            diagnostic_entry.GetType());
  EXPECT_EQ("Confirmation tokens remaining", diagnostic_entry.GetName());
  EXPECT_EQ("0", diagnostic_entry.GetValue());
}

TEST_F(BraveAdsConfirmationTokensRemainingDiagnosticEntryTest, SomeTokens) {
  // Arrange
  test::RefillRandomConfirmationTokens(/*count=*/5);

  const ConfirmationTokensRemainingDiagnosticEntry diagnostic_entry;

  // Act & Assert
  EXPECT_EQ(DiagnosticEntryType::kConfirmationTokensRemaining,
            diagnostic_entry.GetType());
  EXPECT_EQ("Confirmation tokens remaining", diagnostic_entry.GetName());
  EXPECT_EQ("5", diagnostic_entry.GetValue());
}

}  // namespace brave_ads
