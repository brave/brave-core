/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/diagnostics.h"

#include <string>
#include <utility>

#include "base/check_op.h"
#include "base/json/json_writer.h"
#include "bat/ads/internal/diagnostics/diagnostics_util.h"
#include "bat/ads/internal/diagnostics/entries/catalog_id_diagnostic_entry.h"
#include "bat/ads/internal/diagnostics/entries/catalog_last_updated_diagnostic_entry.h"
#include "bat/ads/internal/diagnostics/entries/enabled_diagnostic_entry.h"
#include "bat/ads/internal/diagnostics/entries/last_unidle_time_diagnostic_entry.h"
#include "bat/ads/internal/diagnostics/entries/locale_diagnostic_entry.h"

namespace ads {

namespace {
Diagnostics* g_diagnostics_instance = nullptr;
}  // namespace

Diagnostics::Diagnostics() {
  DCHECK(!g_diagnostics_instance);
  g_diagnostics_instance = this;

  SetEntry(std::make_unique<EnabledDiagnosticEntry>());
  SetEntry(std::make_unique<LocaleDiagnosticEntry>());
  SetEntry(std::make_unique<CatalogIdDiagnosticEntry>());
  SetEntry(std::make_unique<CatalogLastUpdatedDiagnosticEntry>());
  SetEntry(std::make_unique<LastUnIdleTimeDiagnosticEntry>());
}

Diagnostics::~Diagnostics() {
  DCHECK_EQ(this, g_diagnostics_instance);
  g_diagnostics_instance = nullptr;
}

// static
Diagnostics* Diagnostics::Get() {
  DCHECK(g_diagnostics_instance);
  return g_diagnostics_instance;
}

// static
bool Diagnostics::HasInstance() {
  return !!g_diagnostics_instance;
}

void Diagnostics::SetEntry(std::unique_ptr<DiagnosticEntryInterface> entry) {
  DCHECK(entry);

  DiagnosticEntryType type = entry->GetType();
  diagnostics_[type] = std::move(entry);
}

void Diagnostics::Get(GetDiagnosticsCallback callback) const {
  std::string json;
  if (!base::JSONWriter::Write(ToValue(diagnostics_), &json)) {
    callback(/* success */ false, {});
  }

  callback(/* success */ true, json);
}

}  // namespace ads
