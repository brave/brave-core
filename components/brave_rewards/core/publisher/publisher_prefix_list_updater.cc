/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/publisher/publisher_prefix_list_updater.h"

#include <utility>

#include "brave/components/brave_rewards/core/common/prefs.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/publisher/prefix_list_reader.h"
#include "brave/components/brave_rewards/core/rewards_engine.h"
#include "net/http/http_status_code.h"

namespace {

constexpr int64_t kRetryAfterFailureDelay = 150;
constexpr int64_t kMaxRetryAfterFailureDelay = 4 * base::Time::kSecondsPerHour;

}  // namespace

namespace brave_rewards::internal::publisher {

PublisherPrefixListUpdater::PublisherPrefixListUpdater(RewardsEngine& engine)
    : engine_(engine), rewards_server_(engine) {}

PublisherPrefixListUpdater::~PublisherPrefixListUpdater() = default;

void PublisherPrefixListUpdater::StartAutoUpdate(
    PublisherPrefixListUpdatedCallback callback) {
  on_updated_callback_ = std::move(callback);
  auto_update_ = true;
  if (!timer_.IsRunning()) {
    StartFetchTimer(FROM_HERE, GetAutoUpdateDelay());
  }
}

void PublisherPrefixListUpdater::StopAutoUpdate() {
  engine_->Log(FROM_HERE) << "Cancelling publisher prefix list update";
  auto_update_ = false;
  timer_.Stop();
}

void PublisherPrefixListUpdater::StartFetchTimer(
    const base::Location& posted_from,
    base::TimeDelta delay) {
  engine_->Log(FROM_HERE) << "Scheduling publisher prefix list update in "
                          << delay.InSeconds() << " seconds";
  timer_.Start(posted_from, delay,
               base::BindOnce(&PublisherPrefixListUpdater::OnFetchTimerElapsed,
                              base::Unretained(this)));
}

void PublisherPrefixListUpdater::OnFetchTimerElapsed() {
  engine_->Log(FROM_HERE) << "Fetching publisher prefix list";
  rewards_server_.get_prefix_list().Request(
      base::BindOnce(&PublisherPrefixListUpdater::OnFetchCompleted,
                     weak_factory_.GetWeakPtr()));
}

void PublisherPrefixListUpdater::OnFetchCompleted(mojom::Result result,
                                                  std::string body) {
  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE)
        << "Invalid server response for publisher prefix list";
    StartFetchTimer(FROM_HERE, GetRetryAfterFailureDelay());
    return;
  }

  PrefixListReader reader;
  auto parse_error = reader.Parse(body);
  if (parse_error != PrefixListReader::ParseError::kNone) {
    // This could be a problem on the client or the server, but
    // optimistically assume that it is a server issue and retry
    // with back-off.
    engine_->LogError(FROM_HERE) << "Failed to parse publisher prefix list: "
                                 << static_cast<int>(parse_error);
    StartFetchTimer(FROM_HERE, GetRetryAfterFailureDelay());
    return;
  }

  if (reader.empty()) {
    engine_->Log(FROM_HERE)
        << "Publisher prefix list did not contain any values";
    StartFetchTimer(FROM_HERE, GetRetryAfterFailureDelay());
    return;
  }

  retry_count_ = 0;

  engine_->Log(FROM_HERE) << "Resetting publisher prefix list table";
  engine_->database()->ResetPublisherPrefixList(
      std::move(reader),
      base::BindOnce(&PublisherPrefixListUpdater::OnPrefixListInserted,
                     weak_factory_.GetWeakPtr()));
}

void PublisherPrefixListUpdater::OnPrefixListInserted(mojom::Result result) {
  // At this point we have received a valid response from the server
  // and we've attempted to insert it into the database. Store the last
  // successful fetch time for calculation of next refresh interval.
  // In order to avoid unecessary server load, do not attempt to retry
  // using a failure delay if the database insert was unsuccessful.
  engine_->Get<Prefs>().SetUint64(prefs::kServerPublisherListStamp,
                                  util::GetCurrentTimeStamp());

  if (auto_update_) {
    StartFetchTimer(FROM_HERE, GetAutoUpdateDelay());
  }

  if (result != mojom::Result::OK) {
    engine_->LogError(FROM_HERE)
        << "Error updating publisher prefix list table: " << result;
    return;
  }

  if (on_updated_callback_) {
    on_updated_callback_.Run();
  }
}

base::TimeDelta PublisherPrefixListUpdater::GetAutoUpdateDelay() {
  uint64_t last_fetch_sec =
      engine_->Get<Prefs>().GetUint64(prefs::kServerPublisherListStamp);

  auto now = base::Time::Now();
  auto fetch_time = base::Time::FromSecondsSinceUnixEpoch(
      static_cast<double>(last_fetch_sec));

  if (fetch_time > now) {
    fetch_time = now;
  }

  fetch_time += base::Seconds(kRefreshInterval);
  return fetch_time < now ? base::Seconds(0) : fetch_time - now;
}

base::TimeDelta PublisherPrefixListUpdater::GetRetryAfterFailureDelay() {
  return util::GetRandomizedDelayWithBackoff(
      base::Seconds(kRetryAfterFailureDelay),
      base::Seconds(kMaxRetryAfterFailureDelay), retry_count_++);
}

}  // namespace brave_rewards::internal::publisher
