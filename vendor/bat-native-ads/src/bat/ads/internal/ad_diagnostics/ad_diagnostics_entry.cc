/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_entry.h"

#include "base/strings/string_number_conversions.h"

namespace ads {

const char kDiagnosticsEntryName[] = "name";
const char kDiagnosticsEntryValue[] = "value";

const char kDiagnosticsAdsEnabled[] = "Ads enabled";
const char kDiagnosticsAdsInitialized[] = "Ads initialized";
const char kDiagnosticsLocale[] = "Locale";
const char kDiagnosticsCatalogId[] = "Catalog ID";
const char kDiagnosticsCatalogLastUpdated[] = "Catalog last updated";
const char kDiagnosticsLastUnIdleTimestamp[] = "Last unidle timestamp";

template <>
std::string ValueToString(bool value) {
  return value ? "true" : "false";
}

template <>
std::string ValueToString(std::string value) {
  return value;
}

absl::optional<std::string> GetDiagnosticsEntry(const base::Value& diagnostics,
                                                const std::string& name) {
  DCHECK(diagnostics.is_list());

  for (const base::Value& item : diagnostics.GetList()) {
    DCHECK(item.is_dict());
    const std::string* key = item.FindStringKey(kDiagnosticsEntryName);
    if (!key || *key != name) {
      continue;
    }
    const std::string* value = item.FindStringKey(kDiagnosticsEntryValue);
    return value ? *value : absl::optional<std::string>{};
  }

  return absl::optional<std::string>{};
}

}  // namespace ads
