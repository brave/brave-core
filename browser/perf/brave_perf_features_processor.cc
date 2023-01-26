// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/perf/brave_perf_features_processor.h"

#include "base/command_line.h"
#include "base/threading/thread_task_runner_handle.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_browser_process.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/perf/brave_perf_switches.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_today/common/pref_names.h"
#include "brave/components/speedreader/speedreader_pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_switches.h"
#include "components/prefs/pref_service.h"

namespace {
void FakeCallback(ledger::mojom::CreateRewardsWalletResult) {}

void EnableAdblockCookieList(base::WeakPtr<Profile> profile) {
  if (!profile) {
    return;
  }

  // Obtrusive cookie notices list in cosmetic filters.
  auto* regional_service_manager =
      g_brave_browser_process->ad_block_service()->regional_service_manager();
  if (!regional_service_manager ||
      !regional_service_manager->IsFilterListAvailable(
          brave_shields::kCookieListUuid)) {
    base::ThreadTaskRunnerHandle::Get()->PostDelayedTask(
        FROM_HERE, base::BindOnce(&EnableAdblockCookieList, profile),
        base::Seconds(1));
    return;
  }

  regional_service_manager->EnableFilterList(brave_shields::kCookieListUuid,
                                             true);
}

}  // namespace

namespace perf {

void MaybeEnableBraveFeatureForPerfTesting(Profile* profile) {
  auto* cmd = base::CommandLine::ForCurrentProcess();
  if (!cmd->HasSwitch(switches::kEnableBraveFeaturesForPerfTesting) ||
      !cmd->HasSwitch(::switches::kUserDataDir)) {
    return;
  }

  // Ads
  auto* ads_service = brave_ads::AdsServiceFactory::GetForProfile(profile);
  ads_service->SetEnabled(true);

  // Rewards
  auto* rewards_service =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile);
  rewards_service->CreateRewardsWallet("US", base::BindOnce(&FakeCallback));

  // Brave news
  profile->GetPrefs()->SetBoolean(brave_news::prefs::kNewTabPageShowToday,
                                  true);
  profile->GetPrefs()->SetBoolean(brave_news::prefs::kBraveTodayOptedIn, true);

  // Speedreader
  profile->GetPrefs()->SetBoolean(speedreader::kSpeedreaderPrefEnabled, true);

  // Adblock
  EnableAdblockCookieList(profile->GetWeakPtr());
}

}  // namespace perf
