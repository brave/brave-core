/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_TOR_UTILS_H_
#define BRAVE_COMPONENTS_TOR_TOR_UTILS_H_

#include <vector>

#include "base/values.h"

class PrefService;

namespace tor {

struct BridgesConfig {
  BridgesConfig();
  BridgesConfig(BridgesConfig&&) noexcept;
  BridgesConfig(const BridgesConfig&) = delete;
  ~BridgesConfig();

  BridgesConfig& operator=(BridgesConfig&&) noexcept;
  BridgesConfig& operator=(const BridgesConfig&) = delete;

  static absl::optional<BridgesConfig> FromValue(const base::Value* v);
  base::Value::Dict ToDict() const;
  base::Value ToValue() const;

  bool use_bridges = false;
  bool use_builtin_bridges = false;
  std::vector<std::string> bridges;
};

void MigrateLastUsedProfileFromLocalStatePrefs(PrefService* local_state);

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_TOR_UTILS_H_
