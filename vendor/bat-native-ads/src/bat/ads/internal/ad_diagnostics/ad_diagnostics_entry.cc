/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_entry.h"

#include <utility>

#include "base/strings/string_number_conversions.h"
#include "base/values.h"

namespace ads {

namespace {

const char kDiagnosticsEntryName[] = "name";
const char kDiagnosticsEntryValue[] = "value";

}  // namespace

const char kDiagnosticsAdsEnabled[] = "Ads enabled";
const char kDiagnosticsLocale[] = "Locale";
const char kDiagnosticsCatalogId[] = "Catalog ID";
const char kDiagnosticsCatalogLastUpdated[] = "Catalog last updated";
const char kDiagnosticsLastUnIdleTimestamp[] = "Last unidle timestamp";

void AddDiagnosticsEntry(const std::string& name,
                         const std::string& value,
                         base::Value* diagnostics) {
  DCHECK(diagnostics);
  DCHECK(diagnostics->is_list());

  base::Value entry(base::Value::Type::DICTIONARY);
  entry.SetStringKey(kDiagnosticsEntryName, name);
  entry.SetStringKey(kDiagnosticsEntryValue, value);
  diagnostics->Append(std::move(entry));
}

absl::optional<std::string> GetDiagnosticsEntry(const base::Value& diagnostics,
                                                const std::string& name) {
  DCHECK(diagnostics.is_list());

  const std::string* value = nullptr;
  for (const base::Value& item : diagnostics.GetList()) {
    DCHECK(item.is_dict());
    const std::string* key = item.FindStringKey(kDiagnosticsEntryName);
    if (key && *key == name) {
      value = item.FindStringKey(kDiagnosticsEntryValue);
      break;
    }
  }

  return value ? *value : absl::optional<std::string>();
}

}  // namespace ads
