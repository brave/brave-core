/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DIAGNOSTICS_ENTRIES_DEVICE_ID_DIAGNOSTIC_ENTRY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DIAGNOSTICS_ENTRIES_DEVICE_ID_DIAGNOSTIC_ENTRY_H_

#include <string>

#include "bat/ads/internal/diagnostics/diagnostic_entry_interface.h"

namespace ads {

class DeviceIdDiagnosticEntry final : public DiagnosticEntryInterface {
 public:
  DeviceIdDiagnosticEntry();
  DeviceIdDiagnosticEntry(const DeviceIdDiagnosticEntry&) = delete;
  DeviceIdDiagnosticEntry& operator=(const DeviceIdDiagnosticEntry&) = delete;
  ~DeviceIdDiagnosticEntry() override;

  // DiagnosticEntryInterface:
  DiagnosticEntryType GetType() const override;
  std::string GetName() const override;
  std::string GetValue() const override;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DIAGNOSTICS_ENTRIES_DEVICE_ID_DIAGNOSTIC_ENTRY_H_
