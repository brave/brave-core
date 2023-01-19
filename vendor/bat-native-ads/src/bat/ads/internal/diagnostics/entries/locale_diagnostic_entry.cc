/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/diagnostics/entries/locale_diagnostic_entry.h"

#include "brave/components/l10n/common/locale_util.h"

namespace ads {

namespace {
constexpr char kName[] = "Locale";
}  // namespace

DiagnosticEntryType LocaleDiagnosticEntry::GetType() const {
  return DiagnosticEntryType::kLocale;
}

std::string LocaleDiagnosticEntry::GetName() const {
  return kName;
}

std::string LocaleDiagnosticEntry::GetValue() const {
  return brave_l10n::GetDefaultLocaleString();
}

}  // namespace ads
