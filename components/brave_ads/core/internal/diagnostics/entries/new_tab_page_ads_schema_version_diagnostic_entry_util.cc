/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/new_tab_page_ads_schema_version_diagnostic_entry_util.h"

#include <memory>
#include <utility>

#include "brave/components/brave_ads/core/internal/diagnostics/diagnostic_manager.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/new_tab_page_ads_schema_version_diagnostic_entry.h"

namespace brave_ads {

void SetNewTabPageAdsSchemaVersionDiagnosticEntry(int schema_version) {
  auto new_tab_page_ads_schema_version_diagnostic_entry =
      std::make_unique<NewTabPageAdsSchemaVersionDiagnosticEntry>(
          schema_version);

  DiagnosticManager::GetInstance().SetEntry(
      std::move(new_tab_page_ads_schema_version_diagnostic_entry));
}

}  // namespace brave_ads
