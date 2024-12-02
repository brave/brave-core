/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/database/database_maintenance.h"

#include "base/functional/bind.h"
#include "base/time/time.h"
#include "brave/components/brave_ads/core/internal/account/deposits/deposits_database_util.h"
#include "brave/components/brave_ads/core/internal/account/transactions/transactions_database_table_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/creatives/conversions/creative_set_conversion_database_table_util.h"
#include "brave/components/brave_ads/core/internal/database/database_manager.h"
#include "brave/components/brave_ads/core/internal/history/ad_history_database_table_util.h"
#include "brave/components/brave_ads/core/internal/user_engagement/ad_events/ad_events_database_table_util.h"

namespace brave_ads::database {

namespace {

constexpr base::TimeDelta kInitialDelay = base::Minutes(1);
constexpr base::TimeDelta kRecurringInterval = base::Days(1);

void RunOnce() {
  PurgeAllOrphanedAdEvents();
}

}  // namespace

Maintenance::Maintenance() {
  DatabaseManager::GetInstance().AddObserver(this);
}

Maintenance::~Maintenance() {
  DatabaseManager::GetInstance().RemoveObserver(this);
}

///////////////////////////////////////////////////////////////////////////////

void Maintenance::RepeatedlyScheduleAfter(const base::TimeDelta after) {
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

void Maintenance::OnDatabaseIsReady() {
  RunOnce();

  RepeatedlyScheduleAfter(kInitialDelay);
}

}  // namespace brave_ads::database
