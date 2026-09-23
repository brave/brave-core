/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/diagnostics/entries/language_diagnostic_entry.h"

#include "brave/components/brave_ads/core/public/common/locale/locale_util.h"

namespace brave_ads {

namespace {
constexpr char kName[] = "Language";
}  // namespace

DiagnosticEntryType LanguageDiagnosticEntry::GetType() const {
  return DiagnosticEntryType::kLanguage;
}

std::string LanguageDiagnosticEntry::GetName() const {
  return kName;
}

std::string LanguageDiagnosticEntry::GetValue() const {
  return CurrentLanguageCode();
}

}  // namespace brave_ads
