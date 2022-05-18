/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/entries/enabled_diagnostic_entry.h"

#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/base/string_util.h"
#include "bat/ads/pref_names.h"

namespace ads {

namespace {
constexpr char kName[] = "Enabled";
}  // namespace

EnabledDiagnosticEntry::EnabledDiagnosticEntry() = default;

EnabledDiagnosticEntry::~EnabledDiagnosticEntry() = default;

DiagnosticEntryType EnabledDiagnosticEntry::GetType() const {
  return DiagnosticEntryType::kEnabled;
}

std::string EnabledDiagnosticEntry::GetName() const {
  return kName;
}

std::string EnabledDiagnosticEntry::GetValue() const {
  const bool is_enabled =
      AdsClientHelper::Get()->GetBooleanPref(prefs::kEnabled);
  return BoolToString(is_enabled);
}

}  // namespace ads
