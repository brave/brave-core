/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_sync/values_conv.h"
#include "base/logging.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/values.h"
#include "brave/components/brave_sync/settings.h"
#include "brave/components/brave_sync/sync_devices.h"
#include "brave/components/brave_sync/jslib_const.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "components/bookmarks/browser/bookmark_node.h"

namespace brave_sync {

using base::Value;

std::unique_ptr<Value> BraveSyncSettingsToValue(brave_sync::Settings *brave_sync_settings) {
  CHECK(brave_sync_settings);
  auto result = std::make_unique<Value>(Value::Type::DICTIONARY);

  result->SetKey("this_device_name", Value(brave_sync_settings->this_device_name_));
  result->SetKey("this_device_id", Value(brave_sync_settings->this_device_id_));
  result->SetKey("this_device_id_v2", Value(brave_sync_settings->this_device_id_v2_));
  result->SetKey("sync_this_device", Value(brave_sync_settings->sync_this_device_));
  result->SetKey("sync_bookmarks", Value(brave_sync_settings->sync_bookmarks_));
  result->SetKey("sync_settings", Value(brave_sync_settings->sync_settings_));
  result->SetKey("sync_history", Value(brave_sync_settings->sync_history_));

  result->SetKey("sync_configured", Value(brave_sync_settings->sync_configured_));

  return result;
}

template <typename TEnum>
TEnum ConvertEnum(const int ival, TEnum min, TEnum max, TEnum def) {
  DCHECK(ival >= min && ival <= max) << " Unexpected enum value " << ival <<
    " Should be between or include " << min << " and " << max;
  if (ival < min && ival > max) {
    return def;
  }

  return static_cast<TEnum>(ival);
}

template
jslib::SiteSetting::AdControl ConvertEnum<jslib::SiteSetting::AdControl>(const int ival,
  jslib::SiteSetting::AdControl, jslib::SiteSetting::AdControl, jslib::SiteSetting::AdControl);

template
jslib::SiteSetting::CookieControl ConvertEnum<jslib::SiteSetting::CookieControl>(const int ival,
  jslib::SiteSetting::CookieControl, jslib::SiteSetting::CookieControl, jslib::SiteSetting::CookieControl);

template
jslib::SyncRecord::Action ConvertEnum<jslib::SyncRecord::Action>(const int ival,
  jslib::SyncRecord::Action, jslib::SyncRecord::Action, jslib::SyncRecord::Action);


template <typename TEnum>
TEnum ExtractEnum(const base::Value *val, const std::string &fileld_name,
  TEnum min, TEnum max, TEnum def
  ) {
  DCHECK(val);
  DCHECK(!fileld_name.empty());
  DCHECK(val->is_dict());

  const base::Value *int_value = val->FindKeyOfType(fileld_name, base::Value::Type::INTEGER);
  DCHECK(int_value);
  if (!int_value) {
    return def;
  }

  int ival = int_value->GetInt();
  return ConvertEnum<TEnum>(ival, min, max, def);
}

template
jslib::SiteSetting::AdControl ExtractEnum<jslib::SiteSetting::AdControl>(const base::Value *val, const std::string &fileld_name,
  jslib::SiteSetting::AdControl, jslib::SiteSetting::AdControl, jslib::SiteSetting::AdControl);

template
jslib::SiteSetting::CookieControl ExtractEnum<jslib::SiteSetting::CookieControl>(const base::Value *val, const std::string &fileld_name,
  jslib::SiteSetting::CookieControl, jslib::SiteSetting::CookieControl, jslib::SiteSetting::CookieControl);

template
jslib::SyncRecord::Action ExtractEnum<jslib::SyncRecord::Action>(const base::Value *val, const std::string &fileld_name,
  jslib::SyncRecord::Action, jslib::SyncRecord::Action, jslib::SyncRecord::Action);

std::string StrFromUint8Array(const Uint8Array &arr) {
  std::string result;
  for (size_t i = 0; i < arr.size(); ++i) {
    result += base::NumberToString( static_cast<unsigned char>( arr.at(i)));
    if (i != arr.size() - 1) {
      result += ", ";
    }
  }
  return result;
}

std::string StrFromUnsignedCharArray(const std::vector<unsigned char> &vec) {
  return StrFromUint8Array(vec);
}

Uint8Array Uint8ArrayFromString(const std::string &data_string) {
  return UCharVecFromString(data_string);
}

std::vector<unsigned char> UCharVecFromString(const std::string &data_string) {
  std::vector<std::string> splitted = SplitString(
      data_string,
      ", ",
      base::WhitespaceHandling::TRIM_WHITESPACE,
      base::SplitResult::SPLIT_WANT_NONEMPTY);

  std::vector<unsigned char> result;
  result.reserve(splitted.size());

  for (size_t i = 0; i < splitted.size(); ++i) {
    int output = 0;
    bool b = base::StringToInt(splitted[i], &output);
    CHECK(b);
    CHECK(output >= 0 && output <= 255);
    result.emplace_back(static_cast<unsigned char>(output));
  }

  return result;
}

} // namespace brave_sync
