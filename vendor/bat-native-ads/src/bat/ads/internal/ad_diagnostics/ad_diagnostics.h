/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_AD_DIAGNOSTICS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_AD_DIAGNOSTICS_H_

#include "base/time/time.h"
#include "bat/ads/ads.h"

namespace base {
class Value;
}

namespace ads {

class AdDiagnostics final {
 public:
  AdDiagnostics();
  AdDiagnostics(const AdDiagnostics&) = delete;
  AdDiagnostics& operator=(const AdDiagnostics&) = delete;
  ~AdDiagnostics();

  static AdDiagnostics* Get();

  void SetLastUnIdleTimestamp(const base::Time& value);

  void GetAdDiagnostics(GetAdDiagnosticsCallback callback) const;

 private:
  base::Value CollectDiagnostics() const;
  void CollectCatalogDiagnostics(base::Value* diagnostics) const;

  base::Time last_unidle_timestamp_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_AD_DIAGNOSTICS_H_
