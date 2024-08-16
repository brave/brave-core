/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"

#include <utility>

#include "base/check.h"
#include "base/json/values_util.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"

namespace brave_ads {

std::optional<base::Value> GetProfilePref(const std::string& path) {
  return GetAdsClient()->GetProfilePref(path);
}

bool GetProfileBooleanPref(const std::string& path) {
  const std::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return false;
  }

  CHECK(value->is_bool()) << "Wrong type for GetProfileBooleanPref: " << path;

  return value->GetBool();
}

int GetProfileIntegerPref(const std::string& path) {
  const std::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return 0;
  }

  CHECK(value->is_int()) << "Wrong type for GetProfileIntegerPref: " << path;

  return value->GetInt();
}

double GetProfileDoublePref(const std::string& path) {
  const std::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return 0.0;
  }

  CHECK(value->is_double()) << "Wrong type for GetProfileDoublePref: " << path;

  return value->GetDouble();
}

std::string GetProfileStringPref(const std::string& path) {
  const std::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return "";
  }

  CHECK(value->is_string()) << "Wrong type for GetProfileStringPref: " << path;

  return value->GetString();
}

base::Value::Dict GetProfileDictPref(const std::string& path) {
  const std::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return {};
  }

  CHECK(value->is_dict()) << "Wrong type for GetProfileDictPref: " << path;

  return value->GetDict().Clone();
}

base::Value::List GetProfileListPref(const std::string& path) {
  const std::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return {};
  }

  CHECK(value->is_list()) << "Wrong type for GetProfileListPref: " << path;

  return value->GetList().Clone();
}

int64_t GetProfileInt64Pref(const std::string& path) {
  const std::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return 0;
  }

  CHECK(value->is_string()) << "Wrong type for GetProfileInt64Pref: " << path;

  const std::optional<int64_t> integer = base::ValueToInt64(*value);
  return integer.value_or(0);
}

uint64_t GetProfileUint64Pref(const std::string& path) {
  const std::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return 0;
  }

  CHECK(value->is_string()) << "Wrong type for GetProfileUint64Pref: " << path;

  uint64_t integer;
  return base::StringToUint64(value->GetString(), &integer) ? integer : 0;
}

base::Time GetProfileTimePref(const std::string& path) {
  const std::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return {};
  }

  CHECK(value->is_string()) << "Wrong type for GetProfileTimePref: " << path;

  const std::optional<base::Time> time = base::ValueToTime(*value);
  return time.value_or(base::Time());
}

base::TimeDelta GetProfileTimeDeltaPref(const std::string& path) {
  const std::optional<base::Value> value = GetProfilePref(path);
  if (!value) {
    return {};
  }

  CHECK(value->is_string())
      << "Wrong type for GetProfileTimedDeltaPref: " << path;

  const std::optional<base::TimeDelta> time_delta =
      base::ValueToTimeDelta(*value);
  return time_delta.value_or(base::TimeDelta());
}

void SetProfilePref(const std::string& path, base::Value value) {
  GetAdsClient()->SetProfilePref(path, std::move(value));
}

void SetProfileBooleanPref(const std::string& path, const bool value) {
  SetProfilePref(path, base::Value(value));
}

void SetProfileIntegerPref(const std::string& path, const int value) {
  SetProfilePref(path, base::Value(value));
}

void SetProfileDoublePref(const std::string& path, const double value) {
  SetProfilePref(path, base::Value(value));
}

void SetProfileStringPref(const std::string& path, const std::string& value) {
  SetProfilePref(path, base::Value(value));
}

void SetProfileDictPref(const std::string& path, base::Value::Dict value) {
  SetProfilePref(path, base::Value(std::move(value)));
}

void SetProfileListPref(const std::string& path, base::Value::List value) {
  SetProfilePref(path, base::Value(std::move(value)));
}

void SetProfileInt64Pref(const std::string& path, const int64_t value) {
  SetProfilePref(path, base::Int64ToValue(value));
}

void SetProfileUint64Pref(const std::string& path, const uint64_t value) {
  SetProfilePref(path, base::Value(base::NumberToString(value)));
}

void SetProfileTimePref(const std::string& path, const base::Time value) {
  SetProfilePref(path, base::TimeToValue(value));
}

void SetProfileTimeDeltaPref(const std::string& path,
                             const base::TimeDelta value) {
  SetProfilePref(path, base::TimeDeltaToValue(value));
}

void ClearProfilePref(const std::string& path) {
  GetAdsClient()->ClearProfilePref(path);
}

bool HasProfilePrefPath(const std::string& path) {
  return GetAdsClient()->HasProfilePrefPath(path);
}

std::optional<base::Value> GetLocalStatePref(const std::string& path) {
  return GetAdsClient()->GetLocalStatePref(path);
}

bool GetLocalStateBooleanPref(const std::string& path) {
  const std::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return false;
  }

  CHECK(value->is_bool()) << "Wrong type for GetLocalStateBooleanPref: "
                          << path;

  return value->GetBool();
}

int GetLocalStateIntegerPref(const std::string& path) {
  const std::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return 0;
  }

  CHECK(value->is_int()) << "Wrong type for GetLocalStateIntegerPref: " << path;

  return value->GetInt();
}

double GetLocalStateDoublePref(const std::string& path) {
  const std::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return 0.0;
  }

  CHECK(value->is_double())
      << "Wrong type for GetLocalStateDoublePref: " << path;

  return value->GetDouble();
}

std::string GetLocalStateStringPref(const std::string& path) {
  const std::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return "";
  }

  CHECK(value->is_string())
      << "Wrong type for GetLocalStateStringPref: " << path;

  return value->GetString();
}

base::Value::Dict GetLocalStateDictPref(const std::string& path) {
  const std::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return {};
  }

  CHECK(value->is_dict()) << "Wrong type for GetLocalStateDictPref: " << path;

  return value->GetDict().Clone();
}

base::Value::List GetLocalStateListPref(const std::string& path) {
  const std::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return {};
  }

  CHECK(value->is_list()) << "Wrong type for GetLocalStateListPref: " << path;

  return value->GetList().Clone();
}

int64_t GetLocalStateInt64Pref(const std::string& path) {
  const std::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return 0;
  }

  CHECK(value->is_string())
      << "Wrong type for GetLocalStateInt64Pref: " << path;

  const std::optional<int64_t> integer = base::ValueToInt64(*value);
  return integer.value_or(0);
}

uint64_t GetLocalStateUint64Pref(const std::string& path) {
  const std::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return 0;
  }

  CHECK(value->is_string())
      << "Wrong type for GetLocalStateUint64Pref: " << path;

  uint64_t integer;
  return base::StringToUint64(value->GetString(), &integer) ? integer : 0;
}

base::Time GetLocalStateTimePref(const std::string& path) {
  const std::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return {};
  }

  CHECK(value->is_string()) << "Wrong type for GetLocalStateTimePref: " << path;

  const std::optional<base::Time> time = base::ValueToTime(*value);
  return time.value_or(base::Time());
}

base::TimeDelta GetLocalStateTimeDeltaPref(const std::string& path) {
  const std::optional<base::Value> value = GetLocalStatePref(path);
  if (!value) {
    return {};
  }

  CHECK(value->is_string())
      << "Wrong type for GetLocalStateTimedDeltaPref: " << path;

  const std::optional<base::TimeDelta> time_delta =
      base::ValueToTimeDelta(*value);
  return time_delta.value_or(base::TimeDelta());
}

void SetLocalStatePref(const std::string& path, base::Value value) {
  GetAdsClient()->SetLocalStatePref(path, std::move(value));
}

void SetLocalStateBooleanPref(const std::string& path, const bool value) {
  SetLocalStatePref(path, base::Value(value));
}

void SetLocalStateIntegerPref(const std::string& path, const int value) {
  SetLocalStatePref(path, base::Value(value));
}

void SetLocalStateDoublePref(const std::string& path, const double value) {
  SetLocalStatePref(path, base::Value(value));
}

void SetLocalStateStringPref(const std::string& path,
                             const std::string& value) {
  SetLocalStatePref(path, base::Value(value));
}

void SetLocalStateDictPref(const std::string& path, base::Value::Dict value) {
  SetLocalStatePref(path, base::Value(std::move(value)));
}

void SetLocalStateListPref(const std::string& path, base::Value::List value) {
  SetLocalStatePref(path, base::Value(std::move(value)));
}

void SetLocalStateInt64Pref(const std::string& path, const int64_t value) {
  SetLocalStatePref(path, base::Int64ToValue(value));
}

void SetLocalStateUint64Pref(const std::string& path, const uint64_t value) {
  SetLocalStatePref(path, base::Value(base::NumberToString(value)));
}

void SetLocalStateTimePref(const std::string& path, const base::Time value) {
  SetLocalStatePref(path, base::TimeToValue(value));
}

void SetLocalStateTimeDeltaPref(const std::string& path,
                                const base::TimeDelta value) {
  SetLocalStatePref(path, base::TimeDeltaToValue(value));
}

void ClearLocalStatePref(const std::string& path) {
  GetAdsClient()->ClearLocalStatePref(path);
}

bool HasLocalStatePrefPath(const std::string& path) {
  return GetAdsClient()->HasLocalStatePrefPath(path);
}

}  // namespace brave_ads
