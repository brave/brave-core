/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_TOR_TOR_UTILS_H_
#define BRAVE_COMPONENTS_TOR_TOR_UTILS_H_

#include <cstddef>
#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "base/containers/span.h"
#include "base/values.h"

class PrefService;

namespace tor {

// Maximum length of a single bridge line, and the maximum number of bridge
// lines Tor is configured with at once.
inline constexpr size_t kMaxBridgeLineLength = 1024;
inline constexpr size_t kMaxBridgeLines = 64;

// Returns true if `line` matches the grammar Tor's own parse_bridge_line()
// accepts, and contains no character that could break out of the quoted
// argument of a control port SETCONF command. A line that fails this is both
// unusable by Tor and unsafe to forward to it; see TorControl::SetupBridges().
bool IsValidBridgeLine(std::string_view line);

// Returns the entries of `bridges` that Tor can safely be configured with,
// in their original order: anything IsValidBridgeLine() rejects is dropped,
// and at most kMaxBridgeLines are kept. Rejections are logged at VLOG(1).
std::vector<std::string> FilterBridgeLines(
    base::span<const std::string> bridges);

struct BridgesConfig {
  // This enum is used in prefs & UI. Be careful when editing.
  // UI reference: brave_tor_subpage.js
  enum class Usage {
    kNotUsed = 0,
    kBuiltIn = 1,
    kRequest = 2,
    kProvide = 3,
  };

  // This enum is used in prefs & UI. Be careful when editing.
  // UI reference: brave_tor_subpage.js
  enum class BuiltinType {
    kSnowflake = 0,
    kObfs4 = 1,
    kMeekAzure = 2,
  };

  BridgesConfig();
  BridgesConfig(BridgesConfig&&) noexcept;
  BridgesConfig(const BridgesConfig&) = delete;
  ~BridgesConfig();

  BridgesConfig& operator=(BridgesConfig&&) noexcept;
  BridgesConfig& operator=(const BridgesConfig&) = delete;

  const std::vector<std::string>& GetBuiltinBridges() const;
  void UpdateBuiltinBridges(const base::DictValue& dict);

  static std::optional<BridgesConfig> FromDict(const base::DictValue& dict);
  static std::optional<BridgesConfig> FromValue(const base::Value* v);
  base::DictValue ToDict(bool include_builtin = true) const;
  base::Value ToValue(bool include_builtin = true) const;

  Usage use_bridges = Usage::kNotUsed;
  BuiltinType use_builtin = BuiltinType::kObfs4;
  std::map<BuiltinType, std::vector<std::string>> builtin_bridges;
  std::vector<std::string> requested_bridges;
  std::vector<std::string> provided_bridges;
};

void MigrateLastUsedProfileFromLocalStatePrefs(PrefService* local_state);

}  // namespace tor

#endif  // BRAVE_COMPONENTS_TOR_TOR_UTILS_H_
