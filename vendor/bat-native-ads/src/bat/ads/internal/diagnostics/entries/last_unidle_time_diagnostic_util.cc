/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/entries/last_unidle_time_diagnostic_util.h"

#include <memory>
#include <utility>

#include "base/time/time.h"
#include "bat/ads/internal/diagnostics/diagnostic_manager.h"
#include "bat/ads/internal/diagnostics/entries/last_unidle_time_diagnostic_entry.h"

namespace ads {

void SetLastUnIdleTimeDiagnosticEntry() {
  auto last_unidle_time_diagnostic_entry =
      std::make_unique<LastUnIdleTimeDiagnosticEntry>();
  last_unidle_time_diagnostic_entry->SetLastUnIdleTime(base::Time::Now());

  DiagnosticManager::GetInstance()->SetEntry(
      std::move(last_unidle_time_diagnostic_entry));
}

}  // namespace ads
