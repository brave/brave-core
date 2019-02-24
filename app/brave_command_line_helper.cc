/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/app/brave_command_line_helper.h"

#include <sstream>
#include <vector>

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/strings/string_split.h"

BraveCommandLineHelper::BraveCommandLineHelper(base::CommandLine* command_line)
    : command_line_(*command_line) {
  Parse();
}

const std::unordered_set<std::string>&
BraveCommandLineHelper::enabled_features() const {
  return enabled_features_;
}

const std::unordered_set<std::string>&
BraveCommandLineHelper::disabled_features() const {
  return disabled_features_;
}

void BraveCommandLineHelper::ParseCSV(
    const std::string& value,
    std::unordered_set<std::string>* dest) const {
  DCHECK(dest);
  if (value.empty())
    return;
  std::vector<std::string> values = base::SplitString(
      value, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_NONEMPTY);
  dest->insert(std::make_move_iterator(values.begin()),
               std::make_move_iterator(values.end()));
}

void BraveCommandLineHelper::Parse() {
  ParseCSV(command_line_.GetSwitchValueASCII(switches::kEnableFeatures),
           &enabled_features_);
  ParseCSV(command_line_.GetSwitchValueASCII(switches::kDisableFeatures),
           &disabled_features_);
  // Remove enabled features that are also disabled. When processing
  // features, if the same feature is enabled and disabled the latter takes
  // precedence, so there is no point adding to enabled set.
  auto it = enabled_features_.begin();
  while (it != enabled_features_.end()) {
    if (disabled_features_.find(*it) != disabled_features_.end())
      it = enabled_features_.erase(it);
    else
      ++it;
  }
}

void BraveCommandLineHelper::AppendSwitch(const char* switch_key) {
  if (!command_line_.HasSwitch(switch_key))
    command_line_.AppendSwitch(switch_key);
}

void BraveCommandLineHelper::AppendFeatures(
    const std::unordered_set<const char*>& enabled,
    const std::unordered_set<const char*>& disabled) {
  // Assuming that the two passed in sets do not intersect, but in case they do
  // process the disabled set first since disabled features take precedence.
  // Add programmatically disabled features that aren't already enabled.
  for (auto it = disabled.cbegin(); it != disabled.cend(); ++it) {
    if (enabled_features_.find(*it) == enabled_features_.end())
      disabled_features_.insert(*it);
  }
  // Add programmatically enabled features that aren't already disabled.
  for (auto it = enabled.cbegin(); it != enabled.cend(); ++it) {
    if (disabled_features_.find(*it) == disabled_features_.end())
      enabled_features_.insert(*it);
  }
  if (!enabled_features_.empty())
    AppendCSV(switches::kEnableFeatures, enabled_features_);
  if (!disabled_features_.empty())
    AppendCSV(switches::kDisableFeatures, disabled_features_);
}

void BraveCommandLineHelper::AppendCSV(
    const char* switch_key,
    const std::unordered_set<std::string>& values) {
  std::stringstream ss;
  for (auto it = values.cbegin(); it != values.cend(); ++it) {
    if (it != values.cbegin())
      ss << ",";
    ss << *it;
  }
  command_line_.AppendSwitchASCII(switch_key, ss.str());
}
