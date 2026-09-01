/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_RESOURCE_DIAGNOSTIC_ENTRY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_RESOURCE_DIAGNOSTIC_ENTRY_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/diagnostic_entry_interface.h"

namespace brave_ads {

// Reports whether a targeting resource (text classification, purchase
// intent, anti targeting) is currently loaded. Every resource has the same
// `IsLoaded()`/`GetManifestVersion()` shape, so one parameterized entry backs
// all of them rather than a class per resource.
class ResourceDiagnosticEntry final : public DiagnosticEntryInterface {
 public:
  ResourceDiagnosticEntry(DiagnosticEntryType type,
                          std::string name,
                          base::RepeatingCallback<bool()> is_loaded,
                          base::RepeatingCallback<std::optional<std::string>()>
                              get_manifest_version);

  ResourceDiagnosticEntry(const ResourceDiagnosticEntry&) = delete;
  ResourceDiagnosticEntry& operator=(const ResourceDiagnosticEntry&) = delete;

  ~ResourceDiagnosticEntry() override;

  // DiagnosticEntryInterface:
  DiagnosticEntryType GetType() const override;
  std::string GetName() const override;
  std::string GetValue() const override;

 private:
  const DiagnosticEntryType type_;
  const std::string name_;
  const base::RepeatingCallback<bool()> is_loaded_;
  const base::RepeatingCallback<std::optional<std::string>()>
      get_manifest_version_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_RESOURCE_DIAGNOSTIC_ENTRY_H_
