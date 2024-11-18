/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_command_line_switches_util.h"

#include <array>
#include <string_view>

#include "base/check.h"
#include "base/command_line.h"
#include "base/ranges/algorithm.h"

namespace brave_ads {

namespace {
constexpr auto kSwitches =
    std::to_array<std::string_view>({"enable-automation"});
}  // namespace

bool DidOverrideCommandLineSwitches() {
  CHECK(base::CommandLine::InitializedForCurrentProcess());
  const base::CommandLine* const command_line =
      base::CommandLine::ForCurrentProcess();

  return base::ranges::any_of(kSwitches, [command_line](auto switch_string) {
    return command_line->HasSwitch(switch_string);
  });
}

}  // namespace brave_ads
