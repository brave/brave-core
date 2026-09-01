/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/optional_bool_diagnostic_entry.h"

#include <utility>

#include "brave/components/brave_ads/core/internal/common/strings/string_conversions_util.h"

namespace brave_ads {

namespace {
constexpr char kNotApplicable[] = "N/A";
}  // namespace

OptionalBoolDiagnosticEntry::OptionalBoolDiagnosticEntry(
    DiagnosticEntryType type,
    std::string name,
    base::RepeatingCallback<std::optional<bool>()> get_value)
    : type_(type), name_(std::move(name)), get_value_(std::move(get_value)) {}

OptionalBoolDiagnosticEntry::~OptionalBoolDiagnosticEntry() = default;

DiagnosticEntryType OptionalBoolDiagnosticEntry::GetType() const {
  return type_;
}

std::string OptionalBoolDiagnosticEntry::GetName() const {
  return name_;
}

std::string OptionalBoolDiagnosticEntry::GetValue() const {
  const std::optional<bool> value = get_value_.Run();
  if (!value) {
    return kNotApplicable;
  }

  return BoolToString(*value);
}

}  // namespace brave_ads
