/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PREFS_PREF_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PREFS_PREF_UTIL_H_

#include <cstdint>
#include <optional>
#include <string>

#include "base/time/time.h"
#include "base/values.h"

namespace brave_ads {

// Gets profile preference values.

std::optional<base::Value> GetProfilePref(const std::string& path);
bool GetProfileBooleanPref(const std::string& path);
int GetProfileIntegerPref(const std::string& path);
double GetProfileDoublePref(const std::string& path);
std::string GetProfileStringPref(const std::string& path);
base::Value::Dict GetProfileDictPref(const std::string& path);
base::Value::List GetProfileListPref(const std::string& path);
int64_t GetProfileInt64Pref(const std::string& path);
uint64_t GetProfileUint64Pref(const std::string& path);
base::Time GetProfileTimePref(const std::string& path);
base::TimeDelta GetProfileTimeDeltaPref(const std::string& path);

// Sets profile preference values and notifies observers.

void SetProfilePref(const std::string& path, base::Value value);
void SetProfileBooleanPref(const std::string& path, bool value);
void SetProfileIntegerPref(const std::string& path, int value);
void SetProfileDoublePref(const std::string& path, double value);
void SetProfileStringPref(const std::string& path, const std::string& value);
void SetProfileDictPref(const std::string& path, base::Value::Dict value);
void SetProfileListPref(const std::string& path, base::Value::List value);
void SetProfileInt64Pref(const std::string& path, int64_t value);
void SetProfileUint64Pref(const std::string& path, uint64_t value);
void SetProfileTimePref(const std::string& path, base::Time value);
void SetProfileTimeDeltaPref(const std::string& path, base::TimeDelta value);
void ClearProfilePref(const std::string& path);
bool HasProfilePrefPath(const std::string& path);

// Gets local state preference values.

std::optional<base::Value> GetLocalStatePref(const std::string& path);
bool GetLocalStateBooleanPref(const std::string& path);
int GetLocalStateIntegerPref(const std::string& path);
double GetLocalStateDoublePref(const std::string& path);
std::string GetLocalStateStringPref(const std::string& path);
base::Value::Dict GetLocalStateDictPref(const std::string& path);
base::Value::List GetLocalStateListPref(const std::string& path);
int64_t GetLocalStateInt64Pref(const std::string& path);
uint64_t GetLocalStateUint64Pref(const std::string& path);
base::Time GetLocalStateTimePref(const std::string& path);
base::TimeDelta GetLocalStateTimeDeltaPref(const std::string& path);

// Sets local state preference values and notifies observers.

void SetLocalStatePref(const std::string& path, base::Value value);
void SetLocalStateBooleanPref(const std::string& path, bool value);
void SetLocalStateIntegerPref(const std::string& path, int value);
void SetLocalStateDoublePref(const std::string& path, double value);
void SetLocalStateStringPref(const std::string& path, const std::string& value);
void SetLocalStateDictPref(const std::string& path, base::Value::Dict value);
void SetLocalStateListPref(const std::string& path, base::Value::List value);
void SetLocalStateInt64Pref(const std::string& path, int64_t value);
void SetLocalStateUint64Pref(const std::string& path, uint64_t value);
void SetLocalStateTimePref(const std::string& path, base::Time value);
void SetLocalStateTimeDeltaPref(const std::string& path, base::TimeDelta value);
void ClearLocalStatePref(const std::string& path);
bool HasLocalStatePrefPath(const std::string& path);

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_PREFS_PREF_UTIL_H_
