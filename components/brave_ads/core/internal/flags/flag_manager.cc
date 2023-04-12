/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/flags/flag_manager.h"

#include "base/check_op.h"
#include "brave/components/brave_ads/core/internal/ads_client_helper.h"
#include "brave/components/brave_ads/core/internal/flags/debug/debug_command_line_switch_parser_util.h"
#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_command_line_switch_values_util.h"
#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_command_line_switches_util.h"
#include "brave/components/brave_ads/core/internal/flags/did_override/did_override_features_from_command_line_util.h"
#include "brave/components/brave_ads/core/internal/flags/environment/environment_command_line_switch_parser_util.h"
#include "brave/components/brave_ads/core/internal/flags/flag_manager_constants.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

namespace {

bool ShouldForceStagingEnvironment() {
  return AdsClientHelper::GetInstance()->GetBooleanPref(
      brave_rewards::prefs::kUseRewardsStagingServer);
}

EnvironmentType ChooseEnvironmentType() {
  if (ShouldForceStagingEnvironment()) {
    return EnvironmentType::kStaging;
  }

  const absl::optional<EnvironmentType> environment_type =
      ParseEnvironmentCommandLineSwitch();
  return environment_type.value_or(kDefaultEnvironmentType);
}

}  // namespace

FlagManager::FlagManager() {
  Initialize();
}

FlagManager::~FlagManager() {}

// static
FlagManager* FlagManager::GetInstance() {
  DCHECK(GlobalState::GetInstance()->GetFlagManager());
  return GlobalState::GetInstance()->GetFlagManager();
}

///////////////////////////////////////////////////////////////////////////////

void FlagManager::Initialize() {
  should_debug_ = ParseDebugCommandLineSwitch();

  did_override_from_command_line_ = DidOverrideFeaturesFromCommandLine() ||
                                    DidOverrideCommandLineSwitchValues() ||
                                    DidOverrideCommandLineSwitches();

  environment_type_ = ChooseEnvironmentType();
}

}  // namespace brave_ads
