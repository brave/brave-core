/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/publisher/publisher_prefix_list_updater.h"

#include <utility>

#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/publisher/prefix_list_reader.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {

constexpr int64_t kRetryAfterFailureDelay = 150;
constexpr int64_t kMaxRetryAfterFailureDelay = 4 * base::Time::kSecondsPerHour;

}  // namespace

namespace brave_rewards::internal {
namespace publisher {

PublisherPrefixListUpdater::PublisherPrefixListUpdater(
    RewardsEngineImpl& engine)
    : engine_(engine), rewards_server_(engine) {}

PublisherPrefixListUpdater::~PublisherPrefixListUpdater() = default;

void PublisherPrefixListUpdater::StartAutoUpdate(
    PublisherPrefixListUpdatedCallback callback) {
  on_updated_callback_ = callback;
  auto_update_ = true;
  if (!timer_.IsRunning()) {
    StartFetchTimer(FROM_HERE, GetAutoUpdateDelay());
  }
}

void PublisherPrefixListUpdater::StopAutoUpdate() {
  BLOG(1, "Cancelling publisher prefix list update");
  auto_update_ = false;
  timer_.Stop();
}

void PublisherPrefixListUpdater::StartFetchTimer(
    const base::Location& posted_from,
    base::TimeDelta delay) {
  BLOG(1, "Scheduling publisher prefix list update in " << delay.InSeconds()
                                                        << " seconds");
  timer_.Start(posted_from, delay,
               base::BindOnce(&PublisherPrefixListUpdater::OnFetchTimerElapsed,
                              base::Unretained(this)));
}

void PublisherPrefixListUpdater::OnFetchTimerElapsed() {
  BLOG(1, "Fetching publisher prefix list");
  auto url_callback =
      std::bind(&PublisherPrefixListUpdater::OnFetchCompleted, this, _1, _2);
  rewards_server_.get_prefix_list().Request(url_callback);
}

void PublisherPrefixListUpdater::OnFetchCompleted(const mojom::Result result,
                                                  const std::string& body) {
  if (result != mojom::Result::OK) {
    BLOG(0, "Invalid server response for publisher prefix list");
    StartFetchTimer(FROM_HERE, GetRetryAfterFailureDelay());
    return;
  }

  PrefixListReader reader;
  auto parse_error = reader.Parse(body);
  if (parse_error != PrefixListReader::ParseError::kNone) {
    // This could be a problem on the client or the server, but
    // optimistically assume that it is a server issue and retry
    // with back-off.
    BLOG(0, "Failed to parse publisher prefix list: "
                << static_cast<int>(parse_error));
    StartFetchTimer(FROM_HERE, GetRetryAfterFailureDelay());
    return;
  }

  if (reader.empty()) {
    BLOG(1, "Publisher prefix list did not contain any values");
    StartFetchTimer(FROM_HERE, GetRetryAfterFailureDelay());
    return;
  }

  retry_count_ = 0;

  BLOG(1, "Resetting publisher prefix list table");
  engine_->database()->ResetPublisherPrefixList(
      std::move(reader),
      std::bind(&PublisherPrefixListUpdater::OnPrefixListInserted, this, _1));
}

void PublisherPrefixListUpdater::OnPrefixListInserted(
    const mojom::Result result) {
  // At this point we have received a valid response from the server
  // and we've attempted to insert it into the database. Store the last
  // successful fetch time for calculation of next refresh interval.
  // In order to avoid unecessary server load, do not attempt to retry
  // using a failure delay if the database insert was unsuccessful.
  engine_->state()->SetServerPublisherListStamp(util::GetCurrentTimeStamp());

  if (auto_update_) {
    StartFetchTimer(FROM_HERE, GetAutoUpdateDelay());
  }

  if (result != mojom::Result::OK) {
    BLOG(0, "Error updating publisher prefix list table: " << result);
    return;
  }

  if (on_updated_callback_) {
    on_updated_callback_();
  }
}

base::TimeDelta PublisherPrefixListUpdater::GetAutoUpdateDelay() {
  uint64_t last_fetch_sec = engine_->state()->GetServerPublisherListStamp();

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

}  // namespace publisher
}  // namespace brave_rewards::internal
