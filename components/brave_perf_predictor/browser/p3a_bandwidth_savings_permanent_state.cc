/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_perf_predictor/browser/p3a_bandwidth_savings_permanent_state.h"

#include <numeric>
#include <utility>

#include "base/time/clock.h"
#include "base/time/default_clock.h"
#include "brave/components/brave_perf_predictor/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace brave_perf_predictor {

namespace {

constexpr size_t kNumOfSavedDailyUptimes = 7;

}  // namespace

P3ABandwidthSavingsPermanentState::P3ABandwidthSavingsPermanentState(
    PrefService* user_prefs)
    : P3ABandwidthSavingsPermanentState(
          user_prefs,
          std::make_unique<base::DefaultClock>()) {}

P3ABandwidthSavingsPermanentState::P3ABandwidthSavingsPermanentState(
    PrefService* user_prefs,
    std::unique_ptr<base::Clock> clock)
    : clock_(std::move(clock)), user_prefs_(user_prefs) {
  if (user_prefs)
    LoadSavingsDaily();
}

P3ABandwidthSavingsPermanentState::~P3ABandwidthSavingsPermanentState() =
    default;

void P3ABandwidthSavingsPermanentState::AddSavings(uint64_t delta) {
  base::Time now_midnight = clock_->Now().LocalMidnight();
  base::Time last_saved_midnight;

  if (!daily_savings_.empty())
    last_saved_midnight = daily_savings_.front().day;

  if (now_midnight - last_saved_midnight > base::TimeDelta()) {
    // Day changed.
    daily_savings_.emplace_front(DailySaving{now_midnight, delta});
    if (daily_savings_.size() > kNumOfSavedDailyUptimes)
      daily_savings_.pop_back();
  } else {
    daily_savings_.front().saving += delta;
  }

  SaveSavingsDaily();
}

uint64_t P3ABandwidthSavingsPermanentState::GetFullPeriodSavingsBytes() const {
  // We record only saving for last N days.
  const base::Time n_days_ago =
      clock_->Now() - base::TimeDelta::FromDays(kNumOfSavedDailyUptimes);
  return std::accumulate(daily_savings_.begin(), daily_savings_.end(), 0UL,
                         [n_days_ago](const uint64_t acc, const auto& u2) {
                           uint64_t add = 0;
                           // Check only last continious days.
                           if (u2.day > n_days_ago) {
                             add = u2.saving;
                           }
                           return acc + add;
                         });
}

void P3ABandwidthSavingsPermanentState::LoadSavingsDaily() {
  DCHECK(daily_savings_.empty());
  if (!user_prefs_)
    return;
  const base::ListValue* list =
      user_prefs_->GetList(prefs::kBandwidthSavedDailyBytes);
  if (!list)
    return;

  for (auto it = list->begin(); it != list->end(); ++it) {
    const base::Value* day = it->FindKey("day");
    const base::Value* saving = it->FindKey("saving");
    if (!day || !saving || !day->is_double() || !saving->is_double())
      continue;
    if (daily_savings_.size() == kNumOfSavedDailyUptimes)
      break;
    daily_savings_.emplace_back(base::Time::FromDoubleT(day->GetDouble()),
                                static_cast<uint64_t>(saving->GetDouble()));
  }
}

void P3ABandwidthSavingsPermanentState::SaveSavingsDaily() {
  DCHECK(!daily_savings_.empty());
  DCHECK_LE(daily_savings_.size(), kNumOfSavedDailyUptimes);

  if (!user_prefs_)
    return;
  ListPrefUpdate update(user_prefs_, prefs::kBandwidthSavedDailyBytes);
  base::ListValue* list = update.Get();
  list->Clear();
  for (const auto& u : daily_savings_) {
    base::DictionaryValue value;
    value.SetKey("day", base::Value(u.day.ToDoubleT()));
    value.SetKey("saving", base::Value(static_cast<double>(u.saving)));
    list->Append(std::move(value));
  }
}

}  // namespace brave_perf_predictor
