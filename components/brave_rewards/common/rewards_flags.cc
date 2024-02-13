/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/common/rewards_flags.h"

#include <optional>
#include <vector>

#include "base/command_line.h"
#include "base/no_destructor.h"
#include "base/strings/strcat.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"

namespace brave_rewards {

namespace {

bool g_force_parsing_for_testing = false;

constexpr char kSwitchName[] = "rewards";

bool ReadBoolFlag(const std::string& value) {
  std::string lower = base::ToLowerASCII(value);
  return lower == "true" || lower == "1";
}

template <typename F>
std::optional<int> ReadInt(const std::string& value, F fn) {
  int int_value;
  if (base::StringToInt(value, &int_value) && fn(int_value)) {
    return int_value;
  }
  return {};
}

std::optional<int> ReadPositiveInt(const std::string& value) {
  return ReadInt(value, [](int v) { return v > 0; });
}

RewardsFlags Parse(const std::string& input) {
  RewardsFlags flags;

  const std::vector<std::string> entries = base::SplitString(
      input, ",", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

  for (const auto& entry : entries) {
    const std::vector<std::string> values = base::SplitString(
        entry, "=", base::KEEP_WHITESPACE, base::SPLIT_WANT_NONEMPTY);

    const std::string name =
        values.empty() ? "" : base::ToLowerASCII(values[0]);
    const std::string value = values.size() > 1 ? values[1] : "";

    if (name == "staging") {
      // The "staging" flag allows the user to specify the staging or production
      // environment; if the flag is "falsy", the production environment is
      // used.
      flags.environment = ReadBoolFlag(value)
                              ? RewardsFlags::Environment::kStaging
                              : RewardsFlags::Environment::kProduction;
    } else if (name == "development") {
      if (ReadBoolFlag(value) && !flags.environment) {
        flags.environment = RewardsFlags::Environment::kDevelopment;
      }
    } else if (name == "debug") {
      flags.debug = ReadBoolFlag(value);
    } else if (name == "reconcile-interval") {
      flags.reconcile_interval = ReadPositiveInt(value);
    } else if (name == "retry-interval") {
      flags.retry_interval = ReadPositiveInt(value);
    } else if (name == "persist-logs") {
      flags.persist_logs = ReadBoolFlag(value);
    }
  }

  return flags;
}

}  // namespace

void RewardsFlags::SetForceParsingForTesting(bool force_parsing_for_testing) {
  g_force_parsing_for_testing = force_parsing_for_testing;
}

const std::string& RewardsFlags::GetCommandLineSwitchASCII() {
  static base::NoDestructor<std::string> command_line_switch(
      []() -> std::string {
        const auto* const command_line = base::CommandLine::ForCurrentProcess();
        if (!command_line->HasSwitch(kSwitchName)) {
          return {};
        }

        return base::StrCat(
            {kSwitchName, "=", command_line->GetSwitchValueASCII(kSwitchName)});
      }());

  return *command_line_switch;
}

const RewardsFlags& RewardsFlags::ForCurrentProcess() {
  static std::optional<RewardsFlags> parsed_flags;

  if (parsed_flags && !g_force_parsing_for_testing) {
    return *parsed_flags;
  }

  std::string input;

  const auto* const command_line = base::CommandLine::ForCurrentProcess();
  if (command_line->HasSwitch(kSwitchName)) {
    input = command_line->GetSwitchValueASCII(kSwitchName);
  }

  parsed_flags = Parse(input);

  return *parsed_flags;
}

}  // namespace brave_rewards
