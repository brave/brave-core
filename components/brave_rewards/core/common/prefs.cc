/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/common/prefs.h"

#include <utility>

#include "base/json/values_util.h"
#include "base/strings/string_number_conversions.h"

namespace brave_rewards::internal {

Prefs::Prefs(RewardsEngine& engine) : RewardsEngineHelper(engine) {}

Prefs::~Prefs() = default;

void Prefs::SetBoolean(std::string_view path, bool value) {
  Set(path, base::Value(value));
}

bool Prefs::GetBoolean(std::string_view path) {
  return GetValue(path).GetBool();
}

void Prefs::SetInteger(std::string_view path, int value) {
  Set(path, base::Value(value));
}

int Prefs::GetInteger(std::string_view path) {
  return GetValue(path).GetInt();
}

void Prefs::SetDouble(std::string_view path, double value) {
  Set(path, base::Value(value));
}

double Prefs::GetDouble(std::string_view path) {
  return GetValue(path).GetDouble();
}

void Prefs::SetString(std::string_view path, std::string_view value) {
  Set(path, base::Value(value));
}

const std::string& Prefs::GetString(std::string_view path) {
  return GetValue(path).GetString();
}

void Prefs::Set(std::string_view path, const base::Value& value) {
  std::string path_string(path);
  client().SetUserPreferenceValue(path_string, value.Clone());
  values_[path_string] = value.Clone();
}

const base::Value& Prefs::GetValue(std::string_view path) {
  std::string path_string(path);
  base::Value value;
  if (client().GetUserPreferenceValue(path_string, &value)) {
    values_[path_string] = std::move(value);
  }
  return values_[path_string];
}

void Prefs::SetDict(std::string_view path, base::Value::Dict dict) {
  Set(path, base::Value(std::move(dict)));
}

const base::Value::Dict& Prefs::GetDict(std::string_view path) {
  return GetValue(path).GetDict();
}

void Prefs::SetInt64(std::string_view path, int64_t value) {
  Set(path, base::Value(base::NumberToString(value)));
}

int64_t Prefs::GetInt64(std::string_view path) {
  return base::ValueToInt64(GetValue(path)).value_or(0);
}

void Prefs::SetUint64(std::string_view path, uint64_t value) {
  Set(path, base::Value(base::NumberToString(value)));
}

uint64_t Prefs::GetUint64(std::string_view path) {
  uint64_t result = 0;
  base::StringToUint64(GetValue(path).GetString(), &result);
  return result;
}

void Prefs::SetTime(std::string_view path, base::Time value) {
  Set(path, base::TimeToValue(value));
}

base::Time Prefs::GetTime(std::string_view path) {
  return base::ValueToTime(GetValue(path)).value_or(base::Time());
}

void Prefs::SetTimeDelta(std::string_view path, base::TimeDelta value) {
  Set(path, base::TimeDeltaToValue(value));
}

base::TimeDelta Prefs::GetTimeDelta(std::string_view path) {
  return base::ValueToTimeDelta(GetValue(path)).value_or(base::TimeDelta());
}

void Prefs::ClearPref(std::string_view path) {
  client().ClearUserPreferenceValue(std::string(path));
}

}  // namespace brave_rewards::internal
