/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/publisher/publisher_prefix_list_updater.h"

#include <memory>
#include <utility>

#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/publisher/prefix_list_reader.h"
#include "bat/ledger/internal/request/request_publisher.h"
#include "bat/ledger/option_keys.h"
#include "net/http/http_status_code.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {

constexpr int64_t kRetryAfterFailureDelay = 150;
constexpr int64_t kMaxRetryAfterFailureDelay = 4 * base::Time::kSecondsPerHour;

}  // namespace

namespace braveledger_publisher {

PublisherPrefixListUpdater::PublisherPrefixListUpdater(
    bat_ledger::LedgerImpl* ledger)
    : ledger_(ledger),
    rewrads_server_(new ledger::endpoint::RewardsServer(ledger)){}

PublisherPrefixListUpdater::~PublisherPrefixListUpdater() = default;

void PublisherPrefixListUpdater::StartAutoUpdate(
    ledger::PublisherPrefixListUpdatedCallback callback) {
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
  BLOG(1, "Scheduling publisher prefix list update in "
      << delay.InSeconds() << " seconds");
  timer_.Start(posted_from, delay, base::BindOnce(
      &PublisherPrefixListUpdater::OnFetchTimerElapsed,
      base::Unretained(this)));
}

void PublisherPrefixListUpdater::OnFetchTimerElapsed() {
  BLOG(1, "Fetching publisher prefix list");
  auto url_callback = std::bind(&PublisherPrefixListUpdater::OnFetchCompleted,
      this,
      _1,
      _2);
  rewrads_server_->get_prefix_list()->Request(url_callback);
}

void PublisherPrefixListUpdater::OnFetchCompleted(
    const ledger::Result result,
    const std::string& body) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Invalid server response for publisher prefix list");
    StartFetchTimer(FROM_HERE, GetRetryAfterFailureDelay());
    return;
  }

  auto reader = std::make_unique<PrefixListReader>();
  auto parse_error = reader->Parse(body);
  if (parse_error != PrefixListReader::ParseError::kNone) {
    // This could be a problem on the client or the server, but
    // optimistically assume that it is a server issue and retry
    // with back-off.
    BLOG(0, "Failed to parse publisher prefix list: "
        << static_cast<int>(parse_error));
    StartFetchTimer(FROM_HERE, GetRetryAfterFailureDelay());
    return;
  }

  if (reader->empty()) {
    BLOG(1, "Publisher prefix list did not contain any values");
    StartFetchTimer(FROM_HERE, GetRetryAfterFailureDelay());
    return;
  }

  retry_count_ = 0;

  BLOG(1, "Resetting publisher prefix list table");
  ledger_->database()->ResetPublisherPrefixList(
      std::move(reader),
      std::bind(&PublisherPrefixListUpdater::OnPrefixListInserted,
          this,
          _1));
}

void PublisherPrefixListUpdater::OnPrefixListInserted(
    const ledger::Result result) {
  // At this point we have received a valid response from the server
  // and we've attempted to insert it into the database. Store the last
  // successful fetch time for calculation of next refresh interval.
  // In order to avoid unecessary server load, do not attempt to retry
  // using a failure delay if the database insert was unsuccessful.
  ledger_->state()->SetServerPublisherListStamp(
      braveledger_time_util::GetCurrentTimeStamp());

  if (auto_update_) {
    StartFetchTimer(FROM_HERE, GetAutoUpdateDelay());
  }

  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Error updating publisher prefix list table: " << result);
    return;
  }

  if (on_updated_callback_) {
    on_updated_callback_();
  }
}

base::TimeDelta PublisherPrefixListUpdater::GetAutoUpdateDelay() {
  uint64_t last_fetch_sec = ledger_->state()->GetServerPublisherListStamp();
  uint64_t interval_sec = ledger_->ledger_client()->GetUint64Option(
      ledger::kOptionPublisherListRefreshInterval);

  auto now = base::Time::Now();
  auto fetch_time = base::Time::FromDoubleT(
      static_cast<double>(last_fetch_sec));

  if (fetch_time > now) {
    fetch_time = now;
  }

  fetch_time += base::TimeDelta::FromSeconds(interval_sec);
  return fetch_time < now
      ? base::TimeDelta::FromSeconds(0)
      : fetch_time - now;
}

base::TimeDelta PublisherPrefixListUpdater::GetRetryAfterFailureDelay() {
  return braveledger_time_util::GetRandomizedDelayWithBackoff(
      base::TimeDelta::FromSeconds(kRetryAfterFailureDelay),
      base::TimeDelta::FromSeconds(kMaxRetryAfterFailureDelay),
      retry_count_++);
}

}  // namespace braveledger_publisher
