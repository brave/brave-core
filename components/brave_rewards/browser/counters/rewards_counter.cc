// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "base/i18n/time_formatting.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/browser/counters/rewards_counter.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/browser_thread.h"

using brave_rewards::RewardsService;
using brave_rewards::RewardsServiceFactory;

using namespace std::placeholders;

RewardsCounter::RewardsCounter(Profile* profile)
    : profile_(profile),
      weak_ptr_factory_(this) {}

RewardsCounter::~RewardsCounter() {
}

const char* RewardsCounter::GetPrefName() const {
  return brave_rewards::prefs::kRewardsAutoContributeSites;
}

void RewardsCounter::Count() {
  weak_ptr_factory_.InvalidateWeakPtrs();
  RewardsService* rewards_service =
      RewardsServiceFactory::GetForProfile(profile_);
  if (rewards_service) {
    rewards_service->GetAutoContributeCount(
      base::Bind(&RewardsCounter::OnRewardsCounted,
        weak_ptr_factory_.GetWeakPtr()));
  }
}

void RewardsCounter::OnRewardsCounted(
    const ResultInt count,
    uint64_t previous_reconcile_stamp) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);
  ReportResult(std::make_unique<RewardsResult>(
    this, count,
      (previous_reconcile_stamp != 0) ? TimeFormatShortDate(
        base::Time::FromDoubleT(previous_reconcile_stamp)) : base::string16()));
}

RewardsCounter::RewardsResult::RewardsResult(
    const RewardsCounter* source,
    ResultInt site_count,
    base::string16 date)
    : FinishedResult(source, site_count), date_(date) {}

RewardsCounter::RewardsResult::~RewardsResult() {}

base::string16 RewardsCounter::RewardsResult::Date()
    const {
  return date_;
}
