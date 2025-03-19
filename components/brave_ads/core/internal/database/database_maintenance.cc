/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/database/database_maintenance.h"

#include "base/functional/bind.h"
#include "base/location.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposits_database_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table_util.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_database_table_util.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_path_util.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table_util.h"

namespace brave_ads::database {

namespace {

constexpr base::TimeDelta kInitialDelay = base::Minutes(1);
constexpr base::TimeDelta kRecurringInterval = base::Days(1);

void RunOnce() {
  PurgeAllOrphanedAdEvents();
}

// We do not purge ad events for notification ads and search result ads because
// users can only opt out of these ads if they have joined Brave Rewards.

void MaybePurgeNewTabPageAdEvents() {
  if (UserHasJoinedBraveRewards()) {
    // We should not purge ad events if the user has joined Brave Rewards.
    return;
  }

  if (!UserHasOptedInToNewTabPageAds()) {
    PurgeAdEventsForType(mojom::AdType::kNewTabPageAd);
  }
}

void MaybePurgeBraveNewsAdEvents() {
  if (UserHasJoinedBraveRewards()) {
    // We should not purge ad events if the user has joined Brave Rewards.
    return;
  }

  if (!UserHasOptedInToBraveNewsAds()) {
    PurgeAdEventsForType(mojom::AdType::kInlineContentAd);
    PurgeAdEventsForType(mojom::AdType::kPromotedContentAd);
  }
}

}  // namespace

Maintenance::Maintenance() {
  DatabaseManager::GetInstance().AddObserver(this);
}

Maintenance::~Maintenance() {
  DatabaseManager::GetInstance().RemoveObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void Maintenance::RepeatedlyScheduleAfter(base::TimeDelta after) {
  const base::Time at =
      timer_.Start(FROM_HERE, after,
                   base::BindOnce(&Maintenance::RepeatedlyScheduleAfterCallback,
                                  weak_factory_.GetWeakPtr()));

  BLOG(1, "Scheduled database maintenance " << FriendlyDateAndTime(at));
}

void Maintenance::RepeatedlyScheduleAfterCallback() {
  PurgeExpiredAdEvents();
  PurgeExpiredAdHistory();
  PurgeExpiredCreativeSetConversions();
  PurgeExpiredDeposits();
  PurgeExpiredTransactions();

  RepeatedlyScheduleAfter(kRecurringInterval);
}

void Maintenance::OnNotifyPrefDidChange(const std::string& path) {
  if (DoesMatchUserHasOptedInToNewTabPageAdsPrefPath(path)) {
    MaybePurgeNewTabPageAdEvents();
  } else if (DoesMatchUserHasOptedInToBraveNewsAdsPrefPath(path)) {
    MaybePurgeBraveNewsAdEvents();
  }
}

void Maintenance::OnDatabaseIsReady() {
  RunOnce();

  RepeatedlyScheduleAfter(kInitialDelay);
}

}  // namespace brave_ads::database
