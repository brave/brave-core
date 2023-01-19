/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DIAGNOSTICS_ENTRIES_LAST_UNIDLE_TIME_DIAGNOSTIC_ENTRY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DIAGNOSTICS_ENTRIES_LAST_UNIDLE_TIME_DIAGNOSTIC_ENTRY_H_

#include <string>

#include "base/time/time.h"
#include "bat/ads/internal/diagnostics/diagnostic_entry_interface.h"

namespace ads {

class LastUnIdleTimeDiagnosticEntry final : public DiagnosticEntryInterface {
 public:
  void SetLastUnIdleTime(base::Time time);

  // DiagnosticEntryInterface:
  DiagnosticEntryType GetType() const override;
  std::string GetName() const override;
  std::string GetValue() const override;

 private:
  base::Time last_unidle_time_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_DIAGNOSTICS_ENTRIES_LAST_UNIDLE_TIME_DIAGNOSTIC_ENTRY_H_
