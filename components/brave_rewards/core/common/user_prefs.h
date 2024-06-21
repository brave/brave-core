/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_USER_PREFS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_USER_PREFS_H_

#include <string>
#include <string_view>

#include "base/containers/flat_map.h"
#include "base/strings/string_piece.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/brave_rewards/core/rewards_engine_helper.h"

namespace brave_rewards::internal {

// Provides access to registered Rewards user profile prefs. This class
// implements a subset of the `PrefService` interface, and is intended to allow
// access to preferences from a utility process via the `RewardsEngineClient`
// interface.
class UserPrefs : public RewardsEngineHelper, public WithHelperKey<UserPrefs> {
 public:
  explicit UserPrefs(RewardsEngine& engine);
  ~UserPrefs() override;

  void SetBoolean(const std::string& path, bool value);
  bool GetBoolean(const std::string& path);

  void SetInteger(const std::string& path, int value);
  int GetInteger(const std::string& path);

  void SetDouble(const std::string& path, double value);
  double GetDouble(const std::string& path);

  void SetString(const std::string& path, std::string_view value);
  const std::string& GetString(const std::string& path);

  void Set(const std::string& path, const base::Value& value);
  const base::Value& GetValue(const std::string& path);

  void SetDict(const std::string& path, base::Value::Dict dict);
  const base::Value::Dict& GetDict(const std::string& path);

  void SetInt64(const std::string& path, int64_t value);
  int64_t GetInt64(const std::string& path);

  void SetUint64(const std::string& path, uint64_t value);
  uint64_t GetUint64(const std::string& path);

  void SetTime(const std::string& path, base::Time value);
  base::Time GetTime(const std::string& path);

  void SetTimeDelta(const std::string& path, base::TimeDelta value);
  base::TimeDelta GetTimeDelta(const std::string& path);

  void ClearPref(const std::string& path);

 private:
  base::flat_map<std::string, base::Value> values_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_USER_PREFS_H_
