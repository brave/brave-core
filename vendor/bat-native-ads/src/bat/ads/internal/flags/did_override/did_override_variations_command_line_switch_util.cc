/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/flags/did_override/did_override_variations_command_line_switch_util.h"

#include "base/check.h"
#include "base/command_line.h"
#include "components/variations/variations_switches.h"

namespace ads {

bool DidOverrideVariationsCommandLineSwitch() {
  CHECK(base::CommandLine::InitializedForCurrentProcess());
  const base::CommandLine* const command_line =
      base::CommandLine::ForCurrentProcess();

  if (!command_line
           ->GetSwitchValueASCII(variations::switches::kFakeVariationsChannel)
           .empty() ||
      !command_line
           ->GetSwitchValueASCII(
               variations::switches::kVariationsOverrideCountry)
           .empty()) {
    return true;
  }

  return false;
}

}  // namespace ads
