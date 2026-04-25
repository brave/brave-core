/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/common/test/command_line_switch_test_util.h"

#include "base/check.h"
#include "base/command_line.h"
#include "base/strings/strcat.h"
#include "brave/components/brave_ads/core/internal/common/strings/string_strip_util.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/command_line_switch_test_util_internal.h"
#include "brave/components/brave_ads/core/internal/common/test/internal/string_test_util_internal.h"

namespace brave_ads::test {

namespace {

constexpr char kNoCommandLineSwitchKey[] = "WithNoCommandLineSwitch";
constexpr char kNoCommandLineSwitchValue[] = "WithEmptyValue";

}  // namespace

void AppendCommandLineSwitches(
    const CommandLineSwitchList& command_line_switches) {
  if (command_line_switches.empty()) {
    return;
  }

  CHECK(base::CommandLine::InitializedForCurrentProcess());
  base::CommandLine* const command_line =
      base::CommandLine::ForCurrentProcess();

  for (const auto& command_line_switch : command_line_switches) {
    if (!command_line_switch.key.empty()) {
      command_line->AppendSwitchASCII(command_line_switch.key,
                                      command_line_switch.value);
    }
  }

  DidAppendCommandLineSwitches() = true;
}

std::string ToString(const CommandLineSwitchInfo& command_line_switch) {
  if (command_line_switch.key.empty()) {
    return kNoCommandLineSwitchKey;
  }

  const std::string command_line_switch_value =
      command_line_switch.value.empty() ? kNoCommandLineSwitchValue
                                        : command_line_switch.value;

  const std::string command_line_switch_as_string =
      base::StrCat({command_line_switch.key, "=", command_line_switch_value});

  return CapitalizeFirstCharacterOfEachWordAndTrimWhitespace(
      StripNonAlphaNumericCharacters(command_line_switch_as_string));
}

}  // namespace brave_ads::test
