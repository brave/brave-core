/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/device_id_diagnostic_entry.h"

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_base.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_entry_types.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

// npm run test -- brave_unit_tests --filter=BraveAds.*

namespace brave_ads {

class BraveAdsDeviceIdDiagnosticEntryTest : public UnitTestBase {};

TEST_F(BraveAdsDeviceIdDiagnosticEntryTest, GetValue) {
  // Arrange
  auto& sys_info = GlobalState::GetInstance()->SysInfo();
  sys_info.device_id =
      "21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e";

  // Act
  const DeviceIdDiagnosticEntry diagnostic_entry;

  // Assert
  EXPECT_EQ(DiagnosticEntryType::kDeviceId, diagnostic_entry.GetType());
  EXPECT_EQ("Device Id", diagnostic_entry.GetName());
  EXPECT_EQ("21b4677de1a9b4a197ab671a1481d3fcb24f826a4358a05aafbaee5a9a51b57e",
            diagnostic_entry.GetValue());
}

}  // namespace brave_ads
