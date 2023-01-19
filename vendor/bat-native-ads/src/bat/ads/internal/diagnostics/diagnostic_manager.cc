/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/diagnostic_manager.h"

#include <utility>

#include "base/check_op.h"
#include "bat/ads/internal/diagnostics/diagnostic_util.h"
#include "bat/ads/internal/diagnostics/entries/catalog_id_diagnostic_entry.h"
#include "bat/ads/internal/diagnostics/entries/catalog_last_updated_diagnostic_entry.h"
#include "bat/ads/internal/diagnostics/entries/device_id_diagnostic_entry.h"
#include "bat/ads/internal/diagnostics/entries/enabled_diagnostic_entry.h"
#include "bat/ads/internal/diagnostics/entries/last_unidle_time_diagnostic_entry.h"
#include "bat/ads/internal/diagnostics/entries/locale_diagnostic_entry.h"

namespace ads {

namespace {
DiagnosticManager* g_diagnostic_manager_instance = nullptr;
}  // namespace

DiagnosticManager::DiagnosticManager() {
  DCHECK(!g_diagnostic_manager_instance);
  g_diagnostic_manager_instance = this;

  SetEntry(std::make_unique<EnabledDiagnosticEntry>());
  SetEntry(std::make_unique<DeviceIdDiagnosticEntry>());
  SetEntry(std::make_unique<LocaleDiagnosticEntry>());
  SetEntry(std::make_unique<CatalogIdDiagnosticEntry>());
  SetEntry(std::make_unique<CatalogLastUpdatedDiagnosticEntry>());
  SetEntry(std::make_unique<LastUnIdleTimeDiagnosticEntry>());
}

DiagnosticManager::~DiagnosticManager() {
  DCHECK_EQ(this, g_diagnostic_manager_instance);
  g_diagnostic_manager_instance = nullptr;
}

// static
DiagnosticManager* DiagnosticManager::GetInstance() {
  DCHECK(g_diagnostic_manager_instance);
  return g_diagnostic_manager_instance;
}

// static
bool DiagnosticManager::HasInstance() {
  return !!g_diagnostic_manager_instance;
}

void DiagnosticManager::SetEntry(
    std::unique_ptr<DiagnosticEntryInterface> entry) {
  DCHECK(entry);

  const DiagnosticEntryType type = entry->GetType();
  diagnostics_[type] = std::move(entry);
}

void DiagnosticManager::GetDiagnostics(GetDiagnosticsCallback callback) const {
  std::move(callback).Run(ToValue(diagnostics_));
}

}  // namespace ads
