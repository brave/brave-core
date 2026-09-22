/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/resource_diagnostic_entry.h"

#include <utility>

#include "base/notreached.h"
#include "base/strings/strcat.h"

namespace brave_ads {

namespace {
constexpr char kNotLoaded[] = "Not loaded";
constexpr char kFailedToLoad[] = "Failed to load";
constexpr char kUnknownManifestVersion[] = "unknown version";
}  // namespace

ResourceDiagnosticEntry::ResourceDiagnosticEntry(
    DiagnosticEntryType type,
    std::string name,
    base::RepeatingCallback<ResourceLoadStateType()> get_resource_state,
    base::RepeatingCallback<std::optional<std::string>()> get_manifest_version)
    : type_(type),
      name_(std::move(name)),
      get_resource_state_(std::move(get_resource_state)),
      get_manifest_version_(std::move(get_manifest_version)) {}

ResourceDiagnosticEntry::~ResourceDiagnosticEntry() = default;

DiagnosticEntryType ResourceDiagnosticEntry::GetType() const {
  return type_;
}

std::string ResourceDiagnosticEntry::GetName() const {
  return name_;
}

std::string ResourceDiagnosticEntry::GetValue() const {
  switch (get_resource_state_.Run()) {
    case ResourceLoadStateType::kNotLoaded:
      return kNotLoaded;
    case ResourceLoadStateType::kFailedToLoad:
      return kFailedToLoad;
    case ResourceLoadStateType::kLoaded:
      return base::StrCat(
          {"Loaded (",
           get_manifest_version_.Run().value_or(kUnknownManifestVersion),
           ")"});
  }
  NOTREACHED() << "Unexpected value for ResourceLoadStateType: "
               << std::to_underlying(get_resource_state_.Run());
}

}  // namespace brave_ads
