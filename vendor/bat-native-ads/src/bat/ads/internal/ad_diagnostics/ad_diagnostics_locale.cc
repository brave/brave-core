/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_locale.h"
#include "brave/components/l10n/browser/locale_helper.h"

namespace ads {

AdDiagnosticsLocale::AdDiagnosticsLocale() = default;

AdDiagnosticsLocale::~AdDiagnosticsLocale() = default;

AdDiagnosticsEntryType AdDiagnosticsLocale::GetEntryType() const {
  return AdDiagnosticsEntryType::kLocale;
}

std::string AdDiagnosticsLocale::GetKey() const {
  return "Locale";
}

std::string AdDiagnosticsLocale::GetValue() const {
  return brave_l10n::LocaleHelper::GetInstance()->GetLocale();
}

}  // namespace ads
