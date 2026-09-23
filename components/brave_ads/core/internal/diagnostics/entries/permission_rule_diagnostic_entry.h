/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_PERMISSION_RULE_DIAGNOSTIC_ENTRY_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_PERMISSION_RULE_DIAGNOSTIC_ENTRY_H_

#include <string>

#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/diagnostics/entries/diagnostic_entry_interface.h"

namespace brave_ads {

// Reports whether a serving permission rule currently allows ads. Every
// permission rule has the same `bool Has...Permission()` shape, so one
// parameterized entry backs all of them rather than a class per rule.
class PermissionRuleDiagnosticEntry final : public DiagnosticEntryInterface {
 public:
  PermissionRuleDiagnosticEntry(DiagnosticEntryType type,
                                std::string name,
                                base::RepeatingCallback<bool()> has_permission);

  PermissionRuleDiagnosticEntry(const PermissionRuleDiagnosticEntry&) = delete;
  PermissionRuleDiagnosticEntry& operator=(
      const PermissionRuleDiagnosticEntry&) = delete;

  ~PermissionRuleDiagnosticEntry() override;

  // DiagnosticEntryInterface:
  DiagnosticEntryType GetType() const override;
  std::string GetName() const override;
  std::string GetValue() const override;

 private:
  const DiagnosticEntryType type_;
  const std::string name_;
  const base::RepeatingCallback<bool()> has_permission_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_DIAGNOSTICS_ENTRIES_PERMISSION_RULE_DIAGNOSTIC_ENTRY_H_
