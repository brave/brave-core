/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_LAST_UNIDLE_TIME_DIAGNOSTIC_ENTRY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_LAST_UNIDLE_TIME_DIAGNOSTIC_ENTRY_H_

#include <optional>
#include <string>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/diagnostic_entry_interface.h"

namespace brave_ads {

class LastUnIdleTimeDiagnosticEntry final : public DiagnosticEntryInterface {
 public:
  LastUnIdleTimeDiagnosticEntry() = default;
  explicit LastUnIdleTimeDiagnosticEntry(base::Time last_unidle_at);

  // DiagnosticEntryInterface:
  DiagnosticEntryType GetType() const override;
  std::string GetName() const override;
  std::string GetValue() const override;

 private:
  std::optional<base::Time> last_unidle_at_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_LAST_UNIDLE_TIME_DIAGNOSTIC_ENTRY_H_
