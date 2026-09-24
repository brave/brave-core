/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_UTIL_REWARDS_PREFS_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_UTIL_REWARDS_PREFS_H_

#include <string>
#include <string_view>

#include "base/containers/flat_map.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/brave_rewards/core/engine/rewards_engine_helper.h"
#include "brave/components/brave_rewards/core/pref_names.h"

namespace brave_rewards::internal {

// Provides access to registered Rewards user profile prefs. This class
// implements a subset of the `PrefService` interface, and is intended to allow
// access to preferences from a utility process via the `RewardsEngineClient`
// interface.
class RewardsPrefs : public RewardsEngineHelper,
                     public WithHelperKey<RewardsPrefs> {
 public:
  explicit RewardsPrefs(RewardsEngine& engine);
  ~RewardsPrefs() override;

  void SetBoolean(std::string_view path, bool value);
  bool GetBoolean(std::string_view path);

  void SetInteger(std::string_view path, int value);
  int GetInteger(std::string_view path);

  void SetDouble(std::string_view path, double value);
  double GetDouble(std::string_view path);

  void SetString(std::string_view path, std::string_view value);
  const std::string& GetString(std::string_view path);

  void Set(std::string_view path, const base::Value& value);
  const base::Value& GetValue(std::string_view path);

  void SetDict(std::string_view path, base::DictValue dict);
  const base::DictValue& GetDict(std::string_view path);

  void SetInt64(std::string_view path, int64_t value);
  int64_t GetInt64(std::string_view path);

  void SetUint64(std::string_view path, uint64_t value);
  uint64_t GetUint64(std::string_view path);

  void SetTime(std::string_view path, base::Time value);
  base::Time GetTime(std::string_view path);

  void SetTimeDelta(std::string_view path, base::TimeDelta value);
  base::TimeDelta GetTimeDelta(std::string_view path);

  void ClearPref(std::string_view path);

 private:
  base::flat_map<std::string, base::Value> values_;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENGINE_UTIL_REWARDS_PREFS_H_
