/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_ENTRY_INTERFACE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_ENTRY_INTERFACE_H_

#include <string>

#include "bat/ads/internal/diagnostics/diagnostic_entry_types.h"

namespace ads {

class DiagnosticEntryInterface {
 public:
  virtual ~DiagnosticEntryInterface() = default;

  virtual DiagnosticEntryType GetType() const = 0;
  virtual std::string GetName() const = 0;
  virtual std::string GetValue() const = 0;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DIAGNOSTICS_DIAGNOSTIC_ENTRY_INTERFACE_H_
