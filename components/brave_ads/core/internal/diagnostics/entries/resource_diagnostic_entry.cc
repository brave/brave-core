/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/resource_diagnostic_entry.h"

#include <utility>

#include "base/strings/strcat.h"

namespace brave_ads {

namespace {
constexpr char kNotLoaded[] = "Not loaded";
constexpr char kUnknownManifestVersion[] = "unknown version";
}  // namespace

ResourceDiagnosticEntry::ResourceDiagnosticEntry(
    DiagnosticEntryType type,
    std::string name,
    base::RepeatingCallback<bool()> is_loaded,
    base::RepeatingCallback<std::optional<std::string>()> get_manifest_version)
    : type_(type),
      name_(std::move(name)),
      is_loaded_(std::move(is_loaded)),
      get_manifest_version_(std::move(get_manifest_version)) {}

ResourceDiagnosticEntry::~ResourceDiagnosticEntry() = default;

DiagnosticEntryType ResourceDiagnosticEntry::GetType() const {
  return type_;
}

std::string ResourceDiagnosticEntry::GetName() const {
  return name_;
}

std::string ResourceDiagnosticEntry::GetValue() const {
  if (!is_loaded_.Run()) {
    return kNotLoaded;
  }

  return base::StrCat(
      {"Loaded (",
       get_manifest_version_.Run().value_or(kUnknownManifestVersion), ")"});
}

}  // namespace brave_ads
