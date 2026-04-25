/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/last_unidle_time_diagnostic_entry_util.h"

#include <memory>
#include <utility>

#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/last_unidle_time_diagnostic_entry.h"

namespace brave_ads {

void SetLastUnIdleTimeDiagnosticEntry(base::Time last_unidle_at) {
  auto last_unidle_time_diagnostic_entry =
      std::make_unique<LastUnIdleTimeDiagnosticEntry>(last_unidle_at);

  DiagnosticManager::GetInstance().SetEntry(
      std::move(last_unidle_time_diagnostic_entry));
}

}  // namespace brave_ads
