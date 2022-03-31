/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_diagnostics/ad_diagnostics.h"

#include <string>
#include <utility>

#include "base/check.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_entry.h"
#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_util.h"
#include "bat/ads/internal/ad_diagnostics/ads_enabled_ad_diagnostics_entry.h"
#include "bat/ads/internal/ad_diagnostics/catalog_id_ad_diagnostics_entry.h"
#include "bat/ads/internal/ad_diagnostics/catalog_last_updated_ad_diagnostics_entry.h"
#include "bat/ads/internal/ad_diagnostics/last_unidle_timestamp_ad_diagnostics_entry.h"
#include "bat/ads/internal/ad_diagnostics/locale_ad_diagnostics_entry.h"

namespace ads {

namespace {

AdDiagnostics* g_ad_diagnostics = nullptr;

}  // namespace

AdDiagnostics::AdDiagnostics() {
  DCHECK(!g_ad_diagnostics);
  g_ad_diagnostics = this;

  SetDiagnosticsEntry(std::make_unique<AdsEnabledAdDiagnosticsEntry>());
  SetDiagnosticsEntry(std::make_unique<LocaleAdDiagnosticsEntry>());
  SetDiagnosticsEntry(std::make_unique<CatalogIdAdDiagnosticsEntry>());
  SetDiagnosticsEntry(std::make_unique<CatalogLastUpdatedAdDiagnosticsEntry>());
  SetDiagnosticsEntry(
      std::make_unique<LastUnIdleTimestampAdDiagnosticsEntry>());
}

AdDiagnostics::~AdDiagnostics() {
  DCHECK(g_ad_diagnostics);
  g_ad_diagnostics = nullptr;
}

// static
AdDiagnostics* AdDiagnostics::Get() {
  DCHECK(g_ad_diagnostics);
  return g_ad_diagnostics;
}

void AdDiagnostics::SetDiagnosticsEntry(
    std::unique_ptr<AdDiagnosticsEntry> entry) {
  DCHECK(entry);
  AdDiagnosticsEntryType type = entry->GetEntryType();
  ad_diagnostics_entries_[type] = std::move(entry);
}

void AdDiagnostics::GetAdDiagnostics(GetAdDiagnosticsCallback callback) const {
  base::Value diagnostics = CollectDiagnostics();
  std::string json;
  const bool serialized = base::JSONWriter::Write(diagnostics, &json);
  DCHECK(serialized);

  callback(/* success */ true, json);
}

base::Value AdDiagnostics::CollectDiagnostics() const {
  base::Value diagnostics(base::Value::Type::LIST);

  for (const auto& entry_pair : ad_diagnostics_entries_) {
    AdDiagnosticsEntry* entry = entry_pair.second.get();
    AppendDiagnosticsKeyValue(entry->GetKey(), entry->GetValue(), &diagnostics);
  }

  return diagnostics;
}

}  // namespace ads
