/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/permission_rule_diagnostic_entry.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/common/strings/string_conversions_util.h"

namespace brave_ads {

PermissionRuleDiagnosticEntry::PermissionRuleDiagnosticEntry(
    DiagnosticEntryType type,
    std::string name,
    base::RepeatingCallback<bool()> has_permission)
    : type_(type),
      name_(std::move(name)),
      has_permission_(std::move(has_permission)) {}

PermissionRuleDiagnosticEntry::~PermissionRuleDiagnosticEntry() = default;

DiagnosticEntryType PermissionRuleDiagnosticEntry::GetType() const {
  return type_;
}

std::string PermissionRuleDiagnosticEntry::GetName() const {
  return name_;
}

std::string PermissionRuleDiagnosticEntry::GetValue() const {
  return BoolToString(has_permission_.Run());
}

}  // namespace brave_ads
