/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_storage.h"

#include <string_view>
#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/containers/circular_deque.h"
#include "base/time/time.h"
#include "base/values.h"
#include "brave/components/serp_metrics/time_period_storage/serp_metrics_time_period_store.h"

namespace serp_metrics {

namespace {

constexpr std::string_view kDayKey = "day";
constexpr std::string_view kValueKey = "value";

}  // namespace

SerpMetricsTimePeriodStorage::SerpMetricsTimePeriodStorage(
    std::unique_ptr<SerpMetricsTimePeriodStore> store,
    size_t period_days)
    : time_period_store_(std::move(store)), period_days_(period_days) {
  CHECK(time_period_store_);

  Load();
}

SerpMetricsTimePeriodStorage::~SerpMetricsTimePeriodStorage() = default;

void SerpMetricsTimePeriodStorage::AddCount(uint64_t count) {
  FillToToday();
  daily_values_.front().value += count;
  Save();
}

uint64_t SerpMetricsTimePeriodStorage::GetCountForTimeRange(
    const base::Time& start_time,
    const base::Time& end_time) const {
  uint64_t count = 0;
  for (const auto& daily_value : daily_values_) {
    if (daily_value.time >= start_time && daily_value.time <= end_time) {
      count += daily_value.value;
    }
  }
  return count;
}

uint64_t SerpMetricsTimePeriodStorage::GetCount() const {
  const base::Time now = base::Time::Now();
  const base::Time n_days_ago =
      now.UTCMidnight() - base::Days(period_days_ - 1);
  return GetCountForTimeRange(n_days_ago, now);
}

void SerpMetricsTimePeriodStorage::Clear() {
  daily_values_.clear();
  time_period_store_->Clear();
}

void SerpMetricsTimePeriodStorage::FillToToday() {
  const base::Time now_midnight = base::Time::Now().UTCMidnight();

  if (daily_values_.empty()) {
    // No prior data; seed with an empty bucket for today.
    daily_values_.push_front({now_midnight, 0});
    return;
  }

  const base::Time last_recorded_time = daily_values_.front().time;
  base::Time midnight_bucket =
      (last_recorded_time + base::Days(1)).UTCMidnight();
  while (midnight_bucket <= now_midnight) {
    daily_values_.push_front({midnight_bucket, 0});
    if (daily_values_.size() > period_days_) {
      daily_values_.pop_back();
    }
    midnight_bucket = midnight_bucket + base::Days(1);
  }
}

void SerpMetricsTimePeriodStorage::Load() {
  CHECK(daily_values_.empty());

  const base::ListValue* const list = time_period_store_->Get();
  if (!list) {
    return;
  }

  for (const auto& daily_value : *list) {
    if (!daily_value.is_dict()) {
      continue;
    }
    const base::DictValue& dict = daily_value.GetDict();

    std::optional<double> day = dict.FindDouble(kDayKey);
    if (!day) {
      continue;
    }

    std::optional<double> value = dict.FindDouble(kValueKey);
    if (!value) {
      continue;
    }

    if (daily_values_.size() == period_days_) {
      break;
    }

    const base::Time bucket_time = base::Time::FromSecondsSinceUnixEpoch(*day);

    // Legacy buckets carry local-midnight timestamps. Map them to the UTC
    // midnight of the same local calendar day to preserve correct day
    // attribution after the switch to UTC buckets.
    base::Time midnight;
    if (bucket_time == bucket_time.LocalMidnight()) {
      base::Time::Exploded exploded;
      bucket_time.LocalExplode(&exploded);
      if (!base::Time::FromUTCExploded(exploded, &midnight)) {
        midnight = bucket_time.UTCMidnight();
      }
    } else {
      midnight = bucket_time.UTCMidnight();
    }

    daily_values_.push_back({midnight, static_cast<uint64_t>(*value)});
  }
}

void SerpMetricsTimePeriodStorage::Save() {
  CHECK(!daily_values_.empty());
  CHECK_LE(daily_values_.size(), period_days_);

  base::ListValue list;
  for (const auto& daily_value : daily_values_) {
    list.Append(base::DictValue()
                    .Set(kDayKey, daily_value.time.InSecondsFSinceUnixEpoch())
                    .Set(kValueKey, static_cast<double>(daily_value.value)));
  }
  time_period_store_->Set(std::move(list));
}

}  // namespace serp_metrics
