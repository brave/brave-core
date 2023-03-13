/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/device_id_diagnostic_entry.h"

#include "brave/components/brave_ads/common/interfaces/ads.mojom.h"
#include "brave/components/brave_ads/core/sys_info.h"

namespace brave_ads {

namespace {

constexpr char kName[] = "Device Id";
constexpr char kUnknown[] = "Unknown";

}  // namespace

DiagnosticEntryType DeviceIdDiagnosticEntry::GetType() const {
  return DiagnosticEntryType::kDeviceId;
}

std::string DeviceIdDiagnosticEntry::GetName() const {
  return kName;
}

std::string DeviceIdDiagnosticEntry::GetValue() const {
  const std::string& device_id = SysInfo().device_id;

  if (device_id.empty()) {
    return kUnknown;
  }

  return device_id;
}

}  // namespace brave_ads
