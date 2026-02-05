/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/util/rewards_prefs.h"

#include <utility>

#include "base/json/values_util.h"
#include "base/strings/string_number_conversions.h"

namespace brave_rewards::internal {

RewardsPrefs::RewardsPrefs(RewardsEngine& engine)
    : RewardsEngineHelper(engine) {}

RewardsPrefs::~RewardsPrefs() = default;

void RewardsPrefs::SetBoolean(std::string_view path, bool value) {
  Set(path, base::Value(value));
}

bool RewardsPrefs::GetBoolean(std::string_view path) {
  return GetValue(path).GetIfBool().value_or(false);
}

void RewardsPrefs::SetInteger(std::string_view path, int value) {
  Set(path, base::Value(value));
}

int RewardsPrefs::GetInteger(std::string_view path) {
  return GetValue(path).GetIfInt().value_or(0);
}

void RewardsPrefs::SetDouble(std::string_view path, double value) {
  Set(path, base::Value(value));
}

double RewardsPrefs::GetDouble(std::string_view path) {
  return GetValue(path).GetIfDouble().value_or(0);
}

void RewardsPrefs::SetString(std::string_view path, std::string_view value) {
  Set(path, base::Value(value));
}

const std::string& RewardsPrefs::GetString(std::string_view path) {
  static constexpr std::string default_value = "";
  auto* string_value = GetValue(path).GetIfString();
  return string_value ? *string_value : default_value;
}

void RewardsPrefs::Set(std::string_view path, const base::Value& value) {
  std::string path_string(path);
  client().SetUserPreferenceValue(path_string, value.Clone());
  values_[path_string] = value.Clone();
}

const base::Value& RewardsPrefs::GetValue(std::string_view path) {
  std::string path_string(path);
  base::Value value;
  if (client().GetUserPreferenceValue(path_string, &value)) {
    values_[path_string] = std::move(value);
  }
  return values_[path_string];
}

void RewardsPrefs::SetDict(std::string_view path, base::DictValue dict) {
  Set(path, base::Value(std::move(dict)));
}

const base::DictValue& RewardsPrefs::GetDict(std::string_view path) {
  // TODO(https://github.com/brave/brave-browser/issues/48713): This is a case
  // of `-Wexit-time-destructors` violation and `[[clang::no_destroy]]` has been
  // added in the meantime to fix the build error. Remove this attribute and
  // provide a proper fix.
  [[clang::no_destroy]] static const base::DictValue default_value;
  auto* dict = GetValue(path).GetIfDict();
  return dict ? *dict : default_value;
}

void RewardsPrefs::SetInt64(std::string_view path, int64_t value) {
  Set(path, base::Value(base::NumberToString(value)));
}

int64_t RewardsPrefs::GetInt64(std::string_view path) {
  return base::ValueToInt64(GetValue(path)).value_or(0);
}

void RewardsPrefs::SetUint64(std::string_view path, uint64_t value) {
  Set(path, base::Value(base::NumberToString(value)));
}

uint64_t RewardsPrefs::GetUint64(std::string_view path) {
  uint64_t result = 0;
  base::StringToUint64(GetValue(path).GetString(), &result);
  return result;
}

void RewardsPrefs::SetTime(std::string_view path, base::Time value) {
  Set(path, base::TimeToValue(value));
}

base::Time RewardsPrefs::GetTime(std::string_view path) {
  return base::ValueToTime(GetValue(path)).value_or(base::Time());
}

void RewardsPrefs::SetTimeDelta(std::string_view path, base::TimeDelta value) {
  Set(path, base::TimeDeltaToValue(value));
}

base::TimeDelta RewardsPrefs::GetTimeDelta(std::string_view path) {
  return base::ValueToTimeDelta(GetValue(path)).value_or(base::TimeDelta());
}

void RewardsPrefs::ClearPref(std::string_view path) {
  client().ClearUserPreferenceValue(std::string(path));
}

}  // namespace brave_rewards::internal
