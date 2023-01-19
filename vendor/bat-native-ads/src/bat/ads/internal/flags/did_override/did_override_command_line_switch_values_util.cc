/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/flags/did_override/did_override_command_line_switch_values_util.h"

#include "base/check.h"
#include "base/command_line.h"
#include "base/ranges/algorithm.h"
#include "components/variations/variations_switches.h"

namespace ads {

namespace {

const char* const kSwitches[] = {
    variations::switches::kFakeVariationsChannel,
    variations::switches::kVariationsOverrideCountry};

}  // namespace

bool DidOverrideCommandLineSwitchValues() {
  CHECK(base::CommandLine::InitializedForCurrentProcess());
  const base::CommandLine* const command_line =
      base::CommandLine::ForCurrentProcess();

  return base::ranges::any_of(
      kSwitches, [command_line](const auto* switch_string) {
        DCHECK(switch_string);
        return !command_line->GetSwitchValueASCII(switch_string).empty();
      });
}

}  // namespace ads
