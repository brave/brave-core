/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_diagnostics/locale_ad_diagnostics_entry.h"
#include "brave/components/l10n/browser/locale_helper.h"

namespace ads {

LocaleAdDiagnosticsEntry::LocaleAdDiagnosticsEntry() = default;

LocaleAdDiagnosticsEntry::~LocaleAdDiagnosticsEntry() = default;

AdDiagnosticsEntryType LocaleAdDiagnosticsEntry::GetEntryType() const {
  return AdDiagnosticsEntryType::kLocale;
}

std::string LocaleAdDiagnosticsEntry::GetKey() const {
  return "Locale";
}

std::string LocaleAdDiagnosticsEntry::GetValue() const {
  return brave_l10n::LocaleHelper::GetInstance()->GetLocale();
}

}  // namespace ads
