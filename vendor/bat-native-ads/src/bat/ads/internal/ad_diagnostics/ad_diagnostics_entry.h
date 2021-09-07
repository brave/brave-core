/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_AD_DIAGNOSTICS_ENTRY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_AD_DIAGNOSTICS_ENTRY_H_

#include <string>

#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_entry_type.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

class AdDiagnosticsEntry {
 public:
  AdDiagnosticsEntry();
  virtual ~AdDiagnosticsEntry();

  virtual AdDiagnosticsEntryType GetEntryType() const = 0;
  virtual std::string GetKey() const = 0;
  virtual std::string GetValue() const = 0;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_AD_DIAGNOSTICS_ENTRY_H_
