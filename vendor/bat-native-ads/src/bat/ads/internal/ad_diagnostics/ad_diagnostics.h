/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_AD_DIAGNOSTICS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_AD_DIAGNOSTICS_H_

#include <memory>

#include "base/containers/flat_map.h"
#include "bat/ads/ads.h"
#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_entry_type.h"

namespace base {
class Value;
}

namespace ads {

class AdDiagnosticsEntry;

class AdDiagnostics final {
 public:
  AdDiagnostics();
  AdDiagnostics(const AdDiagnostics&) = delete;
  AdDiagnostics& operator=(const AdDiagnostics&) = delete;
  ~AdDiagnostics();

  static AdDiagnostics* Get();

  void SetDiagnosticsEntry(std::unique_ptr<AdDiagnosticsEntry> entry);
  void GetAdDiagnostics(GetAdDiagnosticsCallback callback) const;

 private:
  base::Value CollectDiagnostics() const;

  base::flat_map<AdDiagnosticsEntryType, std::unique_ptr<AdDiagnosticsEntry>>
      ad_diagnostics_entries_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_AD_DIAGNOSTICS_H_
