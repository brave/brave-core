/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_AD_DIAGNOSTICS_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_AD_DIAGNOSTICS_H_

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

  void GetAdDiagnostics(GetAdDiagnosticsCallback callback) const;

  void set_ads_initialized(const bool value) { ads_initialized_ = value; }
  void set_last_unidle_timestamp(const base::Time& value) {
    last_unidle_timestamp_ = value;
  }

 private:
  base::Value CollectDiagnostics() const;
  void CollectCatalogDiagnostics(base::Value* diagnostics) const;

  bool ads_initialized_ = false;
  base::Time last_unidle_timestamp_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_AD_DIAGNOSTICS_H_
