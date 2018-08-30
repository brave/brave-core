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
#include "brave/components/brave_sync/devices.h"
#include "brave/components/brave_sync/jslib_const.h"
#include "brave/components/brave_sync/jslib_messages.h"
#include "components/bookmarks/browser/bookmark_node.h"
#include "brave/components/brave_sync/value_debug.h"

namespace brave_sync {

using base::Value;

std::unique_ptr<Value> BraveSyncSettingsToValue(brave_sync::Settings *brave_sync_settings) {
  CHECK(brave_sync_settings);
  auto result = std::make_unique<Value>(Value::Type::DICTIONARY);

  result->SetKey("this_device_name", Value(brave_sync_settings->this_device_name_));
  result->SetKey("sync_this_device", Value(brave_sync_settings->sync_this_device_));
  result->SetKey("sync_bookmarks", Value(brave_sync_settings->sync_bookmarks_));
  result->SetKey("sync_settings", Value(brave_sync_settings->sync_settings_));
  result->SetKey("sync_history", Value(brave_sync_settings->sync_history_));

  result->SetKey("sync_configured", Value(brave_sync_settings->sync_configured_));

  return result;
}

std::string ExtractIdFieldFromDict(const base::Value *val, const std::string &fileld_name);
std::string ExtractIdFieldFromList(const base::Value *val, const std::string &fileld_name);

std::string ExtractIdFieldFromDictOrList(const base::Value *val, const std::string &fileld_name) {
  CHECK(val);
  DCHECK(val->is_dict());

  const base::Value *val_field = val->FindKey(fileld_name);
  LOG(ERROR) << "TAGAB ExtractIdFieldFromDictOrList val_field="<<val_field;
  DCHECK(val_field);

  if (val_field->is_dict()) {
    return ExtractIdFieldFromDict(val, fileld_name);
  } else if (val->is_dict()) {
    return ExtractIdFieldFromList(val, fileld_name);
  }
  NOTREACHED();
  return "";
}

std::string ExtractIdFieldFromDict(const base::Value *val, const std::string &fileld_name) {
  CHECK(val);
  DCHECK(val->is_dict());
  LOG(ERROR) << "TAGAB ExtractIdFieldFromDict val=" << brave::debug::ToPrintableString(*val);
  if (!val->is_dict()) {
    return "";
  }
  const base::Value *p_object_id = val->FindKey(fileld_name);
  LOG(ERROR) << "TAGAB ExtractIdFieldFromDict p_object_data="<<p_object_id;
  DCHECK(p_object_id);
  if (p_object_id == nullptr || p_object_id->is_none()) {
    return "";
  }

  //iterate
  std::string object_id;
  for (int i = 0; i < 16; ++i) {
      const base::Value *p_byte = p_object_id->FindKey({base::IntToString(i)});
      if (p_byte == nullptr) {
        return "";
      }
      object_id += base::IntToString(p_byte->GetInt());
      if (i != 15) {object_id += ", ";}
  }

  return object_id;
}

std::string ExtractObjectIdFromDict(const base::Value *val) {
  return ExtractIdFieldFromDict(val, "objectId");
}

std::string ExtractDeviceIdFromDict(const base::Value *val) {
  return ExtractIdFieldFromDict(val, "deviceId");
}

base::Time ExtractTimeFieldFromDict(const base::Value *val, const std::string &time_fileld_name) {
  CHECK(val);
  DCHECK(val->is_dict());
  base::Time time;

  const base::Value *time_value = val->FindKeyOfType(time_fileld_name, base::Value::Type::DOUBLE);
  if (time_value) {
    double d_time = time_value->GetDouble();
    time = base::Time::FromJsTime(d_time);
  } else {
    const base::Value *int_time_value = val->FindKeyOfType(time_fileld_name, base::Value::Type::INTEGER);
    DCHECK(int_time_value);
    int int_time = int_time_value->GetInt();
    time = base::Time::FromJsTime(int_time);
  }

  return time;
}

bool ExtractBool(const base::Value *val, const std::string &fileld_name) {
  const base::Value *bool_value = val->FindKeyOfType(fileld_name, base::Value::Type::BOOLEAN);
  DCHECK(bool_value);
  bool result = bool_value->GetBool();
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

std::string ExtractObjectIdFromList(const base::Value *val) {
  return ExtractIdFieldFromList(val, "objectId");
}

std::string ExtractIdFieldFromList(const base::Value *val, const std::string &fileld_name) {
  CHECK(val);
  DCHECK(val->is_dict());
  LOG(ERROR) << "TAGAB ExtractIdFieldFromList val=" << brave::debug::ToPrintableString(*val);
  if (!val->is_dict()) {
    return "";
  }
  const base::Value *p_object_id = val->FindKey(fileld_name);
  LOG(ERROR) << "TAGAB ExtractIdFieldFromList p_object_data="<<p_object_id;
  DCHECK(p_object_id);
  if (p_object_id == nullptr) {
    return "";
  }

  DCHECK(p_object_id->is_list());

  std::string object_id;
  for (size_t i = 0; i < p_object_id->GetList().size(); ++i) {
      const base::Value &p_byte = p_object_id->GetList().at(i);
      DCHECK(p_byte.is_int());

      object_id += base::IntToString(p_byte.GetInt());
      if (i != p_object_id->GetList().size() - 1) {object_id += ", ";}
  }

  return object_id;
}

std::unique_ptr<base::Value> VecToListValue(const std::vector<char> &v) {
  auto lv = std::make_unique<base::Value>(base::Value::Type::LIST);
  for (const char &i : v) {
    lv->GetList().emplace_back(i);
  }
  return lv;
}

Uint8Array Uint8ArrayFromSignedCharVec(const std::vector<char> &vec) {
  Uint8Array result;
  result.reserve(vec.size());
  for (size_t i = 0; i < vec.size(); ++i) {
    result.emplace_back(static_cast<unsigned char>(vec.at(i)));
  }
  return result;
}

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

std::string StrFromCharArray(const std::vector<char> &vec) {
  std::string result;
  for (size_t i = 0; i < vec.size(); ++i) {
    result += base::NumberToString( static_cast<unsigned char>( vec.at(i)));
    if (i != vec.size() - 1) {
      result += ", ";
    }
  }
  return result;
}

Uint8Array Uint8ArrayFromIntVec(const std::vector<int> vec) {
  Uint8Array result;
  result.reserve(vec.size());
  for (size_t i = 0; i < vec.size(); ++i) {
    result.emplace_back(base::checked_cast<unsigned char>(vec.at(i)));
  }
  return result;
}

Uint8Array Uint8ArrayFromString(const std::string &data_string) {
 std::vector<std::string> splitted = SplitString(
     data_string,
     ", ",
     base::WhitespaceHandling::TRIM_WHITESPACE,
     base::SplitResult::SPLIT_WANT_NONEMPTY);

 Uint8Array result;
 result.reserve(splitted.size());

 for (size_t i = 0; i < splitted.size(); ++i) {
   int output = 0;
   bool b = base::StringToInt(splitted[i], &output);
   CHECK(b);
   result.emplace_back(base::checked_cast<unsigned char>(output));
 }

 return result;
}

std::vector<int> IntVecFromString(const std::string &data_string) {
  std::vector<std::string> splitted = SplitString(
      data_string,
      ", ",
      base::WhitespaceHandling::TRIM_WHITESPACE,
      base::SplitResult::SPLIT_WANT_NONEMPTY);

  std::vector<int> result;
  result.reserve(splitted.size());

  for (size_t i = 0; i < splitted.size(); ++i) {
    int output = 0;
    bool b = base::StringToInt(splitted[i], &output);
    CHECK(b);
    result.emplace_back(output);
  }

  return result;
}

std::vector<char> CharVecFromString(const std::string &data_string) {
  std::vector<std::string> splitted = SplitString(
      data_string,
      ", ",
      base::WhitespaceHandling::TRIM_WHITESPACE,
      base::SplitResult::SPLIT_WANT_NONEMPTY);

  std::vector<char> result;
  result.reserve(splitted.size());

  for (size_t i = 0; i < splitted.size(); ++i) {
    int output = 0;
    bool b = base::StringToInt(splitted[i], &output);
    CHECK(b);
    CHECK(output >= 0 && output <= 255);
    result.emplace_back(static_cast<char>(output));
  }

  return result;
}

std::unique_ptr<base::Value> BytesListFromString(const std::string &data_string) {
  auto value = std::make_unique<base::Value>(base::Value::Type::LIST);

  std::vector<std::string> splitted = SplitString(
      data_string,
      ", ",
      base::WhitespaceHandling::TRIM_WHITESPACE,
      base::SplitResult::SPLIT_WANT_NONEMPTY);

  for (size_t i = 0; i < splitted.size(); ++i) {
    int output = 0;
    bool b = base::StringToInt(splitted[i], &output);
    CHECK(b);
    value->GetList().emplace_back(base::Value(output));
  }

  return value;
}

std::unique_ptr<base::Value> SingleIntStrToListValue(const std::string &data_string) {
  auto value = std::make_unique<base::Value>(base::Value::Type::LIST);
  int output = 0;
  bool b = base::StringToInt(data_string, &output);
  CHECK(b);
  value->GetList().emplace_back(base::Value(output));

  return value;
}

std::unique_ptr<base::Value> BlobFromString(const std::string &data_string) {
  std::vector<std::string> splitted = SplitString(
      data_string,
      ", ",
      base::WhitespaceHandling::TRIM_WHITESPACE,
      base::SplitResult::SPLIT_WANT_NONEMPTY);

  base::Value::BlobStorage vec;
  vec.reserve(splitted.size());
  for (size_t i = 0; i < splitted.size(); ++i) {
    int output = 0;
    bool b = base::StringToInt(splitted[i], &output);
    CHECK(b);
    unsigned char byte = base::checked_cast<unsigned char>(output);
    vec.push_back(byte);
  }

  return std::make_unique<base::Value>(vec);
}

std::unique_ptr<base::Value> BlobFromSingleIntStr(const std::string &data_string) {
 int output = 0;
 bool b = base::StringToInt(data_string, &output);
 CHECK(b);
 unsigned char byte = base::checked_cast<unsigned char>(output);
 base::Value::BlobStorage vec({byte});
 return std::make_unique<base::Value>(vec);
}

std::string GetAction(const base::Value &sync_record) {
  const int iaction = GetIntAction(sync_record);
  if (iaction == -1) {
    return "";
  }

  std::string action = base::IntToString(iaction);
  CHECK(action == jslib_const::CREATE_RECORD || action == jslib_const::UPDATE_RECORD
    || action == jslib_const::DELETE_RECORD);
  return action;
}

int GetIntAction(const base::Value &sync_record) {
  CHECK(sync_record.is_dict());

  const base::Value* val_action = sync_record.FindKey("action");
  DCHECK(val_action != nullptr);
  if (val_action == nullptr) {
    return -1;
  }
  DCHECK(val_action->is_int());
  if (!val_action->is_int()) {
    return -1;
  }

  int iaction = val_action->GetInt();
  DCHECK(iaction == jslib_const::kActionCreate ||
    iaction == jslib_const::kActionUpdate || iaction == jslib_const::kActionDelete);

  return iaction;
}

} // namespace brave_sync
