/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/browser/rewards_service.h"

#include <algorithm>
#include <array>
#include <string>

#include "base/logging.h"
#include "base/time/time.h"
#include "brave/components/brave_rewards/browser/rewards_notification_service_impl.h"
#include "brave/components/brave_rewards/browser/rewards_service_observer.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "brave/components/ipfs/buildflags/buildflags.h"
#include "build/build_config.h"
#include "components/prefs/pref_registry_simple.h"
#include "net/base/registry_controlled_domains/registry_controlled_domain.h"

#if BUILDFLAG(ENABLE_IPFS)
#include "brave/components/ipfs/ipfs_constants.h"
#include "brave/components/ipfs/ipfs_utils.h"
#endif

namespace {

const std::array<std::string, 6> kGreaselionDomains = {
    "twitter.com", "github.com", "reddit.com",
    "twitch.tv",   "vimeo.com",  "youtube.com"};

bool IsGreaselionURL(const GURL& url) {
  return std::any_of(
      kGreaselionDomains.begin(), kGreaselionDomains.end(),
      [&url](auto& domain) {
        return net::registry_controlled_domains::SameDomainOrHost(
            url, GURL("https://" + domain),
            net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
      });
}

}  // namespace

namespace brave_rewards {

RewardsService::RewardsService() = default;

RewardsService::~RewardsService() = default;

void RewardsService::AddObserver(RewardsServiceObserver* observer) {
  observers_.AddObserver(observer);
}

void RewardsService::RemoveObserver(RewardsServiceObserver* observer) {
  observers_.RemoveObserver(observer);
}

std::string RewardsService::GetPublisherIdFromURL(const GURL& url) {
  if (IsGreaselionURL(url)) {
    return "";
  }

#if BUILDFLAG(ENABLE_IPFS)
  if (url.SchemeIs(ipfs::kIPNSScheme)) {
    return ipfs::GetRegistryDomainFromIPNS(url);
  }
#endif

  if (!url.SchemeIsHTTPOrHTTPS()) {
    return "";
  }

  return net::registry_controlled_domains::GetDomainAndRegistry(
      url, net::registry_controlled_domains::INCLUDE_PRIVATE_REGISTRIES);
}

// static
void RewardsService::RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterStringPref(prefs::kNotifications, "");
  registry->RegisterTimeDeltaPref(prefs::kNotificationTimerInterval,
                                  base::Days(1));
  registry->RegisterTimeDeltaPref(prefs::kBackupNotificationInterval,
                                  base::Days(30));
  registry->RegisterTimeDeltaPref(prefs::kNotificationStartupDelay,
                                  base::Seconds(30));
  registry->RegisterBooleanPref(prefs::kBackupSucceeded, false);
  registry->RegisterBooleanPref(prefs::kUserHasFunded, false);
  registry->RegisterBooleanPref(prefs::kUserHasClaimedGrant, false);
  registry->RegisterTimePref(prefs::kAddFundsNotification, base::Time());
  registry->RegisterBooleanPref(prefs::kEnabled, false);
  registry->RegisterTimePref(prefs::kAdsEnabledTimestamp, base::Time());
  registry->RegisterTimeDeltaPref(prefs::kAdsEnabledTimeDelta,
                                  base::TimeDelta());
  registry->RegisterDictionaryPref(prefs::kExternalWallets);
  registry->RegisterUint64Pref(prefs::kServerPublisherListStamp, 0ull);
  registry->RegisterStringPref(prefs::kUpholdAnonAddress, "");
  registry->RegisterStringPref(prefs::kBadgeText, "1");
#if BUILDFLAG(IS_ANDROID)
  registry->RegisterBooleanPref(prefs::kUseRewardsStagingServer, false);
#endif
  registry->RegisterStringPref(prefs::kExternalWalletType, "");
  registry->RegisterUint64Pref(prefs::kPromotionLastFetchStamp, 0ull);
  registry->RegisterBooleanPref(prefs::kPromotionCorruptedMigrated, false);
  registry->RegisterBooleanPref(prefs::kAnonTransferChecked, false);
  registry->RegisterIntegerPref(prefs::kVersion, 0);
  registry->RegisterIntegerPref(prefs::kMinVisitTime, 8);
  registry->RegisterIntegerPref(prefs::kMinVisits, 1);
  registry->RegisterBooleanPref(prefs::kAllowNonVerified, true);
  registry->RegisterBooleanPref(prefs::kAllowVideoContribution, true);
  registry->RegisterDoublePref(prefs::kScoreA, 0.0);
  registry->RegisterDoublePref(prefs::kScoreB, 0.0);
  registry->RegisterBooleanPref(prefs::kAutoContributeEnabled, false);
  registry->RegisterDoublePref(prefs::kAutoContributeAmount, 0.0);
  registry->RegisterUint64Pref(prefs::kNextReconcileStamp, 0ull);
  registry->RegisterUint64Pref(prefs::kCreationStamp, 0ull);
  registry->RegisterStringPref(prefs::kRecoverySeed, "");
  registry->RegisterStringPref(prefs::kPaymentId, "");
  registry->RegisterBooleanPref(prefs::kInlineTipRedditEnabled, true);
  registry->RegisterBooleanPref(prefs::kInlineTipTwitterEnabled, true);
  registry->RegisterBooleanPref(prefs::kInlineTipGithubEnabled, true);
  registry->RegisterDoublePref(prefs::kParametersRate, 0.0);
  registry->RegisterDoublePref(
      prefs::kParametersAutoContributeChoice,
      0.0);
  registry->RegisterStringPref(
      prefs::kParametersAutoContributeChoices,
      "");
  registry->RegisterStringPref(prefs::kParametersTipChoices, "");
  registry->RegisterStringPref(prefs::kParametersMonthlyTipChoices, "");
  registry->RegisterBooleanPref(prefs::kFetchOldBalance, true);
  registry->RegisterBooleanPref(prefs::kEmptyBalanceChecked, false);
  registry->RegisterStringPref(prefs::kWalletBrave, "");
  registry->RegisterStringPref(prefs::kWalletUphold, "");
  registry->RegisterStringPref(prefs::kWalletBitflyer, "");
  registry->RegisterStringPref(prefs::kWalletGemini, "");
}

}  // namespace brave_rewards
