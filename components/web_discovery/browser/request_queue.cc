/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/web_discovery/browser/request_queue.h"

#include <utility>

#include "base/rand_util.h"
#include "brave/components/web_discovery/browser/util.h"
#include "components/prefs/scoped_user_pref_update.h"

namespace web_discovery {

namespace {

constexpr char kRequestTimeKey[] = "request_time";
constexpr char kRetriesKey[] = "retries";
constexpr char kDataKey[] = "data";

}  // namespace

RequestQueue::RequestQueue(
    PrefService* profile_prefs,
    const char* list_pref_name,
    base::TimeDelta request_max_age,
    base::TimeDelta min_request_interval,
    base::TimeDelta max_request_interval,
    size_t max_retries,
    base::RepeatingCallback<void(const base::Value&)> start_request_callback)
    : profile_prefs_(profile_prefs),
      list_pref_name_(list_pref_name),
      backoff_entry_(&kBackoffPolicy),
      request_max_age_(request_max_age),
      min_request_interval_(min_request_interval),
      max_request_interval_(max_request_interval),
      max_retries_(max_retries),
      start_request_callback_(start_request_callback) {
  StartFetchTimer(false);
}

RequestQueue::~RequestQueue() = default;

void RequestQueue::ScheduleRequest(base::Value request_data) {
  base::Value::Dict fetch_dict;
  fetch_dict.Set(kDataKey, std::move(request_data));
  fetch_dict.Set(kRequestTimeKey,
                 static_cast<double>(base::Time::Now().ToTimeT()));

  ScopedListPrefUpdate update(profile_prefs_, list_pref_name_);
  update->Append(std::move(fetch_dict));

  if (!fetch_timer_.IsRunning()) {
    StartFetchTimer(false);
  }
}

std::optional<base::Value> RequestQueue::NotifyRequestComplete(bool success) {
  backoff_entry_.InformOfRequest(success);

  ScopedListPrefUpdate update(profile_prefs_, list_pref_name_);
  auto& request_dict = update->front().GetDict();

  std::optional<base::Value> removed_value;
  bool use_backoff_delta = false;
  bool should_remove = success;

  if (!success) {
    use_backoff_delta = true;
    auto retries = request_dict.FindInt(kRetriesKey);
    if (retries && retries >= max_retries_) {
      should_remove = true;
    } else {
      request_dict.Set(kRetriesKey, retries.value_or(0) + 1);
    }
  }

  if (should_remove) {
    auto* data = request_dict.Find(kDataKey);
    removed_value = data ? data->Clone() : base::Value();
    update->erase(update->begin());
  }

  StartFetchTimer(use_backoff_delta);
  return removed_value;
}

void RequestQueue::OnFetchTimer() {
  ScopedListPrefUpdate update(profile_prefs_, list_pref_name_);
  for (auto it = update->begin(); it != update->end();) {
    const auto* fetch_dict = it->GetIfDict();
    const auto request_time =
        fetch_dict ? fetch_dict->FindDouble(kRequestTimeKey) : std::nullopt;
    const auto* data = fetch_dict ? fetch_dict->Find(kDataKey) : nullptr;
    if (!request_time ||
        (base::Time::Now() - base::Time::FromTimeT(static_cast<time_t>(
                                 *request_time))) > request_max_age_ ||
        !data) {
      it = update->erase(it);
      continue;
    }
    start_request_callback_.Run(*data);
    return;
  }
}

void RequestQueue::StartFetchTimer(bool use_backoff_delta) {
  base::TimeDelta delta;
  if (use_backoff_delta) {
    delta = backoff_entry_.GetTimeUntilRelease();
  } else {
    delta = base::RandTimeDelta(min_request_interval_, max_request_interval_);
  }
  fetch_timer_.Start(
      FROM_HERE, delta,
      base::BindOnce(&RequestQueue::OnFetchTimer, base::Unretained(this)));
}

}  // namespace web_discovery
