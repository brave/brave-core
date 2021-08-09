/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_diagnostics/ad_diagnostics.h"

#include <string>

#include "base/i18n/time_formatting.h"
#include "base/json/json_writer.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_entry.h"
#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/pref_names.h"
#include "brave/components/l10n/browser/locale_helper.h"

namespace ads {

namespace {

std::string TimeToString(const base::Time& time) {
  if (time.is_null())
    return {};
  return base::UTF16ToUTF8(base::TimeFormatShortDateAndTime(time));
}

}  // namespace

AdDiagnostics::AdDiagnostics() = default;

AdDiagnostics::~AdDiagnostics() = default;

void AdDiagnostics::GetAdDiagnostics(GetAdDiagnosticsCallback callback) const {
  base::Value diagnostics = CollectDiagnostics();
  std::string json;
  const bool serialized = base::JSONWriter::Write(diagnostics, &json);
  DCHECK(serialized);

  callback(/* success */ true, json);
}

base::Value AdDiagnostics::CollectDiagnostics() const {
  base::Value diagnostics(base::Value::Type::LIST);

  AddDiagnosticsEntry(kDiagnosticsAdsEnabled,
                      AdsClientHelper::Get()->GetBooleanPref(prefs::kEnabled),
                      &diagnostics);

  AddDiagnosticsEntry(kDiagnosticsAdsInitialized, ads_initialized_,
                      &diagnostics);

  AddDiagnosticsEntry(kDiagnosticsLocale,
                      brave_l10n::LocaleHelper::GetInstance()->GetLocale(),
                      &diagnostics);

  CollectCatalogDiagnostics(&diagnostics);

  AddDiagnosticsEntry(kDiagnosticsLastUnIdleTimestamp,
                      TimeToString(last_unidle_timestamp_), &diagnostics);

  return diagnostics;
}

void AdDiagnostics::CollectCatalogDiagnostics(base::Value* diagnostics) const {
  DCHECK(diagnostics);

  AddDiagnosticsEntry(kDiagnosticsCatalogId,
                      AdsClientHelper::Get()->GetStringPref(prefs::kCatalogId),
                      diagnostics);

  const int64_t catalog_last_updated =
      AdsClientHelper::Get()->GetInt64Pref(prefs::kCatalogLastUpdated);
  const base::Time time = base::Time::FromDoubleT(catalog_last_updated);
  AddDiagnosticsEntry(kDiagnosticsCatalogLastUpdated, TimeToString(time),
                      diagnostics);
}

}  // namespace ads
