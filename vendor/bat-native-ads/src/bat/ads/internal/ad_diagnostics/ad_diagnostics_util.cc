/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/ad_diagnostics/ad_diagnostics_util.h"

#include <utility>

#include "base/check.h"
#include "base/i18n/time_formatting.h"
#include "base/strings/utf_string_conversions.h"
#include "base/time/time.h"
#include "base/values.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {

namespace {

const char kDiagnosticsEntryKey[] = "key";
const char kDiagnosticsEntryValue[] = "value";

}  // namespace

void AppendDiagnosticsKeyValue(const std::string& key,
                               const std::string& value,
                               base::Value* diagnostics) {
  DCHECK(diagnostics);
  DCHECK(diagnostics->is_list());

  base::Value entry(base::Value::Type::DICTIONARY);
  entry.SetStringKey(kDiagnosticsEntryKey, key);
  entry.SetStringKey(kDiagnosticsEntryValue, value);
  diagnostics->Append(std::move(entry));
}

absl::optional<std::string> GetDiagnosticsValueByKey(
    const base::Value& diagnostics,
    const std::string& key) {
  DCHECK(diagnostics.is_list());

  const std::string* value = nullptr;
  for (const base::Value& item : diagnostics.GetListDeprecated()) {
    DCHECK(item.is_dict());
    const std::string* found_key = item.FindStringKey(kDiagnosticsEntryKey);
    if (found_key && *found_key == key) {
      value = item.FindStringKey(kDiagnosticsEntryValue);
      break;
    }
  }

  return value ? *value : absl::optional<std::string>();
}

std::string ConvertToString(const bool value) {
  return value ? "true" : "false";
}

std::string ConvertToString(const base::Time& time) {
  if (time.is_null())
    return {};
  return base::UTF16ToUTF8(base::TimeFormatShortDateAndTime(time));
}

}  // namespace ads
