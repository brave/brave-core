/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/entries/device_id_diagnostic_entry.h"

#include "bat/ads/internal/common/unittest/unittest_base.h"
#include "bat/ads/internal/diagnostics/diagnostic_entry_types.h"
#include "bat/ads/sys_info.h"

// npm run test -- brave_unit_tests --filter=BatAds.*

namespace ads {

class BatAdsDeviceIdDiagnosticEntryTest : public UnitTestBase {};

TEST_F(BatAdsDeviceIdDiagnosticEntryTest, GetValue) {
  // Arrange
  SysInfo().device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  // Act
  const DeviceIdDiagnosticEntry diagnostic_entry;

  // Assert
  EXPECT_EQ(DiagnosticEntryType::kDeviceId, diagnostic_entry.GetType());
  EXPECT_EQ("Device Id", diagnostic_entry.GetName());
  EXPECT_EQ("21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e",
            diagnostic_entry.GetValue());
}

}  // namespace ads
