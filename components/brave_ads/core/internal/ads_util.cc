/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/public/ads_util.h"

#include "brave/components/brave_ads/core/internal/command_line_switches/command_line_switches_constants.h"
#include "brave/components/brave_ads/core/internal/command_line_switches/environment/environment_command_line_switch_parser_util.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "brave/components/brave_ads/core/public/common/locale/locale_util.h"
#include "brave/components/l10n/common/ofac_sanction_util.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(IS_ANDROID)
#include "brave/components/brave_rewards/core/pref_names.h"
#endif  // BUILDFLAG(IS_ANDROID)

namespace brave_ads {

bool IsStagingEnvironment([[maybe_unused]] PrefService& prefs) {
#if BUILDFLAG(IS_ANDROID)
  // TODO(https://github.com/brave/brave-browser/issues/52800): Remove
  // `kUseRewardsStagingServer` pref and use command-line flag on Android
  // instead.
  if (prefs.GetBoolean(brave_rewards::prefs::kUseRewardsStagingServer)) {
    // On Android, this pref is controlled by a UI switch that lets developers
    // force the staging server regardless of the environment.
    return true;
  }
#endif  // BUILDFLAG(IS_ANDROID)

  // Development environment was removed from Ads some time ago, but Rewards
  // still has a development environment. Any non-production environment
  // (development or staging) should therefore fall back to staging behavior.
  const mojom::EnvironmentType environment_type =
      ParseEnvironmentCommandLineSwitch().value_or(kDefaultEnvironmentType);
  return environment_type == mojom::EnvironmentType::kStaging;
}

bool IsSupportedRegion() {
  return !brave_l10n::IsISOCountryCodeOFACSanctioned(CurrentCountryCode());
}

}  // namespace brave_ads
