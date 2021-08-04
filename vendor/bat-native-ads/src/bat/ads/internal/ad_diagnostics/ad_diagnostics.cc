/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_diagnostics/ad_diagnostics.h"

#include <string>
#include <utility>

#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_ads_enabled.h"
#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_catalog_id.h"
#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_catalog_last_updated.h"
#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_entry.h"
#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_last_unidle_timestamp.h"
#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_locale.h"
#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_util.h"

namespace ads {

namespace {

AdDiagnostics* g_ad_diagnostics = nullptr;

}  // namespace

AdDiagnostics::AdDiagnostics() {
  DCHECK(!g_ad_diagnostics);
  g_ad_diagnostics = this;

  SetDiagnosticsEntry(std::make_unique<AdDiagnosticsAdsEnabled>());
  SetDiagnosticsEntry(std::make_unique<AdDiagnosticsLocale>());
  SetDiagnosticsEntry(std::make_unique<AdDiagnosticsCatalogId>());
  SetDiagnosticsEntry(std::make_unique<AdDiagnosticsCatalogLastUpdated>());
  SetDiagnosticsEntry(std::make_unique<AdDiagnosticsLastUnIdleTimestamp>());
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
