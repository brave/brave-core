/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/common/rewards_flags.h"

#include <vector>

#include "base/command_line.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

namespace brave_rewards {

namespace {

constexpr char kSwitchName[] = "rewards";

bool ReadBoolFlag(const std::string& value) {
  std::string lower = base::ToLowerASCII(value);
  return lower == "true" || lower == "1";
}

template <typename F>
absl::optional<int> ReadInt(const std::string& value, F fn) {
  int int_value;
  if (base::StringToInt(value, &int_value) && fn(int_value)) {
    return int_value;
  }
  return {};
}

absl::optional<int> ReadInt(const std::string& value) {
  return ReadInt(value, [](int v) { return true; });
}

absl::optional<int> ReadPositiveInt(const std::string& value) {
  return ReadInt(value, [](int v) { return v > 0; });
}

}  // namespace

RewardsFlags RewardsFlags::Parse(const std::string& input) {
  RewardsFlags flags;

  std::vector<std::string> entries = base::SplitString(
      input, ",", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  for (auto& entry : entries) {
    std::vector<std::string> values = base::SplitString(
        entry, "=", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

    std::string name = values.empty() ? "" : base::ToLowerASCII(values[0]);
    std::string value = values.size() > 1 ? values[1] : "";

    if (name == "staging") {
      // The "staging" flag allows the user to specify the staging or production
      // environment; if the flag is "falsy", the production environment is
      // used.
      flags.environment = ReadBoolFlag(value) ? Environment::kStaging
                                              : Environment::kProduction;
    } else if (name == "development") {
      if (ReadBoolFlag(value) && !flags.environment) {
        flags.environment = Environment::kDevelopment;
      }
    } else if (name == "debug") {
      flags.debug = ReadBoolFlag(value);
    } else if (name == "reconcile-interval") {
      flags.reconcile_interval = ReadPositiveInt(value);
    } else if (name == "retry-interval") {
      flags.retry_interval = ReadPositiveInt(value);
    } else if (name == "gemini-retries") {
      flags.gemini_retries = ReadInt(value, [](int v) { return v >= 0; });
    } else if (name == "persist-logs") {
      flags.persist_logs = ReadBoolFlag(value);
    } else if (name == "countryid") {
      flags.country_id = ReadInt(value);
    }
  }

  return flags;
}

const RewardsFlags& RewardsFlags::ForCurrentProcess() {
  static RewardsFlags parsed_flags = [] {
    std::string input;

    const auto* command_line = base::CommandLine::ForCurrentProcess();
    if (command_line->HasSwitch(kSwitchName)) {
      input = command_line->GetSwitchValueASCII(kSwitchName);
    }

    return Parse(input);
  }();

  return parsed_flags;
}

}  // namespace brave_rewards
