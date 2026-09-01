/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_OPTIONAL_BOOL_DIAGNOSTIC_ENTRY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_OPTIONAL_BOOL_DIAGNOSTIC_ENTRY_H_

#include <optional>
#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/diagnostic_entry_interface.h"

namespace brave_ads {

// Reports "N/A" when the underlying fact isn't available yet (e.g. no wallet
// or issuers fetched this session), else Yes/No. Shared by every diagnostic
// that reduces to a single optional bool rather than a class per fact.
class OptionalBoolDiagnosticEntry final : public DiagnosticEntryInterface {
 public:
  OptionalBoolDiagnosticEntry(
      DiagnosticEntryType type,
      std::string name,
      base::RepeatingCallback<std::optional<bool>()> get_value);

  OptionalBoolDiagnosticEntry(const OptionalBoolDiagnosticEntry&) = delete;
  OptionalBoolDiagnosticEntry& operator=(const OptionalBoolDiagnosticEntry&) =
      delete;

  ~OptionalBoolDiagnosticEntry() override;

  // DiagnosticEntryInterface:
  DiagnosticEntryType GetType() const override;
  std::string GetName() const override;
  std::string GetValue() const override;

 private:
  const DiagnosticEntryType type_;
  const std::string name_;
  const base::RepeatingCallback<std::optional<bool>()> get_value_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_OPTIONAL_BOOL_DIAGNOSTIC_ENTRY_H_
