/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/flags/flags_util.h"

#include "brave/components/brave_ads/core/internal/flags/debug/debug_command_line_switch_parser_util.h"
#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_command_line_switch_values_util.h"
#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_command_line_switches_util.h"
#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_features_from_command_line_util.h"
#include "brave/components/brave_ads/core/internal/flags/environment/environment_command_line_switch_parser_util.h"
#include "brave/components/brave_ads/core/internal/flags/flag_constants.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"

namespace brave_ads {

namespace {

mojom::EnvironmentType ChooseEnvironmentType() {
  return ParseEnvironmentCommandLineSwitch().value_or(kDefaultEnvironmentType);
}

}  // namespace

mojom::FlagsPtr BuildFlags() {
  mojom::FlagsPtr mojom_flags = mojom::Flags::New();

  mojom_flags->should_debug = ParseDebugCommandLineSwitch();

  mojom_flags->did_override_from_command_line =
      DidOverrideFeaturesFromCommandLine() ||
      DidOverrideCommandLineSwitchValues() || DidOverrideCommandLineSwitches();

  mojom_flags->environment_type = ChooseEnvironmentType();

  return mojom_flags;
}

}  // namespace brave_ads
