/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_AD_DIAGNOSTICS_ENTRY_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_AD_DIAGNOSTICS_ENTRY_H_

#include <string>

#include "base/values.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

extern const char kDiagnosticsEntryName[];
extern const char kDiagnosticsEntryValue[];
extern const char kDiagnosticsAdsEnabled[];
extern const char kDiagnosticsAdsInitialized[];
extern const char kDiagnosticsLocale[];
extern const char kDiagnosticsCatalogId[];
extern const char kDiagnosticsCatalogLastUpdated[];
extern const char kDiagnosticsLastUnIdleTimestamp[];

template <typename T>
std::string ValueToString(T value);

template <typename T>
void AddDiagnosticsEntry(const std::string& name,
                         T value,
                         base::Value* diagnostics) {
  DCHECK(diagnostics);
  DCHECK(diagnostics->is_list());

  base::Value entry(base::Value::Type::DICTIONARY);
  entry.SetStringKey(kDiagnosticsEntryName, name);
  entry.SetStringKey(kDiagnosticsEntryValue, ValueToString(value));
  diagnostics->Append(std::move(entry));
}

absl::optional<std::string> GetDiagnosticsEntry(const base::Value& diagnostics,
                                                const std::string& name);

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_AD_DIAGNOSTICS_AD_DIAGNOSTICS_ENTRY_H_
