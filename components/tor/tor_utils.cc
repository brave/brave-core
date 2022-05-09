/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/tor/tor_utils.h"

#include <string>

#include "absl/types/optional.h"
#include "base/files/file_path.h"
#include "base/logging.h"
#include "base/values.h"
#include "brave/components/tor/tor_constants.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace {
constexpr const char kUseBridgesKey[] = "use_bridges";
constexpr const char kUseBuiltinBridgesKey[] = "use_builtin_bridges";
constexpr const char kBridgesKey[] = "bridges";
}  // namespace

namespace tor {

BridgesConfig::BridgesConfig() = default;
BridgesConfig::BridgesConfig(BridgesConfig&&) noexcept = default;
BridgesConfig::~BridgesConfig() = default;

BridgesConfig& BridgesConfig::operator=(BridgesConfig&&) noexcept = default;

// static
absl::optional<BridgesConfig> BridgesConfig::FromValue(const base::Value* v) {
  if (!v || !v->is_dict())
    return absl::nullopt;

  LOG(ERROR) << v->DebugString();

  const auto& dict = v->GetDict();

  BridgesConfig result;
  result.use_bridges = dict.FindBool(kUseBridgesKey).value_or(false);
  result.use_builtin_bridges =
      dict.FindBool(kUseBuiltinBridgesKey).value_or(false);

  if (auto* bridges = dict.FindList(kBridgesKey)) {
    for (const auto& bridge : *bridges) {
      if (!bridge.is_string())
        continue;
      result.bridges.push_back(bridge.GetString());
    }
  }

  return result;
}

base::Value::Dict BridgesConfig::ToDict() const {
  base::Value::Dict result;
  result.Set(kUseBridgesKey, use_bridges);
  result.Set(kUseBuiltinBridgesKey, use_builtin_bridges);

  base::Value::List list;
  for (const auto& bridge : this->bridges) {
    list.Append(bridge);
  }
  result.Set(kBridgesKey, std::move(list));
  return result;
}

base::Value BridgesConfig::ToValue() const {
  return base::Value(ToDict());
}

void MigrateLastUsedProfileFromLocalStatePrefs(PrefService* local_state) {
  // Do this for legacy tor profile migration because tor profile might be last
  // active profile before upgrading
  std::string last_used_profile_name =
      local_state->GetString(prefs::kProfileLastUsed);
  if (!last_used_profile_name.empty() &&
      last_used_profile_name ==
          base::FilePath(tor::kTorProfileDir).AsUTF8Unsafe()) {
    local_state->SetString(prefs::kProfileLastUsed, chrome::kInitialProfile);
  }
}

}  // namespace tor
