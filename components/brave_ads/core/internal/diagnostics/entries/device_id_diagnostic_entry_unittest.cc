/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/device_id_diagnostic_entry.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_constants.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_mock_util.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_entry_types.h"

// npm run test -- brave_unit_tests --filter=BraveAds.*

namespace brave_ads {

class BraveAdsDeviceIdDiagnosticEntryTest : public UnitTestBase {};

TEST_F(BraveAdsDeviceIdDiagnosticEntryTest, GetValue) {
  // Arrange
  MockDeviceId();

  const DeviceIdDiagnosticEntry diagnostic_entry;

  // Act & Assert
  EXPECT_EQ(DiagnosticEntryType::kDeviceId, diagnostic_entry.GetType());
  EXPECT_EQ("Device Id", diagnostic_entry.GetName());
  EXPECT_EQ(kDeviceId, diagnostic_entry.GetValue());
}

}  // namespace brave_ads
