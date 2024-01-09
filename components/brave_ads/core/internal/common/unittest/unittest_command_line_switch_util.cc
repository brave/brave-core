/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/unittest/unittest_command_line_switch_util.h"

#include "base/check.h"
#include "base/command_line.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_ads/core/internal/common/strings/string_strip_util.h"
#include "brave/components/brave_ads/core/internal/common/unittest/unittest_string_util.h"
#include "brave/components/brave_rewards/common/rewards_flags.h"

namespace brave_ads {

namespace {

constexpr char kNoCommandLineSwitchKey[] = "WithNoCommandLineSwitch";
constexpr char kNoCommandLineSwitchValue[] = "WithEmptyValue";

std::string ToString(const CommandLineSwitchInfo& command_line_switch) {
  const std::string switch_value = command_line_switch.value.empty()
                                       ? kNoCommandLineSwitchValue
                                       : command_line_switch.value;

  return base::StrCat({command_line_switch.key, "=", switch_value});
}

std::string SanitizeCommandLineSwitchFromString(
    const std::string& command_line_switch) {
  return CapitalizeFirstCharacterOfEachWordAndTrimWhitespace(
      StripNonAlphaNumericCharacters(command_line_switch));
}

}  // namespace

void InitializeCommandLineSwitches() {
  brave_rewards::RewardsFlags::SetForceParsingForTesting(true);
}

void ShutdownCommandLineSwitches() {
  brave_rewards::RewardsFlags::SetForceParsingForTesting(false);
}

std::optional<bool>& DidAppendCommandLineSwitches() {
  static std::optional<bool> command_line;
  return command_line;
}

void AppendCommandLineSwitches(
    const CommandLineSwitchList& command_line_switches) {
  if (command_line_switches.empty()) {
    return;
  }

  CHECK(base::CommandLine::InitializedForCurrentProcess());
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();

  for (const auto& command_line_switch : command_line_switches) {
    if (command_line_switch.key.empty()) {
      continue;
    }

    command_line->AppendSwitchASCII(command_line_switch.key,
                                    command_line_switch.value);
  }

  DidAppendCommandLineSwitches() = true;
}

std::string SanitizeCommandLineSwitch(
    const CommandLineSwitchInfo& command_line_switch) {
  if (command_line_switch.key.empty()) {
    return kNoCommandLineSwitchKey;
  }

  return SanitizeCommandLineSwitchFromString(ToString(command_line_switch));
}

}  // namespace brave_ads
