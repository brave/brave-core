/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/publisher/server_publisher_fetcher.h"

#include <utility>

#include "base/big_endian.h"
#include "base/json/json_reader.h"
#include "base/strings/string_piece.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/publisher/prefix_util.h"
#include "bat/ledger/internal/publisher/protos/channel_response.pb.h"
#include "bat/ledger/internal/request/request_publisher.h"
#include "bat/ledger/internal/response/response_publisher.h"
#include "bat/ledger/option_keys.h"
#include "brave_base/random.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace {

constexpr size_t kQueryPrefixBytes = 2;

int64_t GetCacheExpiryInSeconds(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  // NOTE: We are reusing the publisher prefix list refresh interval for
  // determining the cache lifetime of publisher details. At a later
  // time we may want to introduce an additional option for this value.
  return ledger->ledger_client()->GetUint64Option(
      ledger::kOptionPublisherListRefreshInterval);
}

}  // namespace

namespace braveledger_publisher {

ServerPublisherFetcher::ServerPublisherFetcher(
    bat_ledger::LedgerImpl* ledger)
    : ledger_(ledger) {
  DCHECK(ledger);
}

ServerPublisherFetcher::~ServerPublisherFetcher() = default;

void ServerPublisherFetcher::Fetch(
    const std::string& publisher_key,
    ledger::GetServerPublisherInfoCallback callback) {
  FetchCallbackVector& callbacks = callback_map_[publisher_key];
  callbacks.push_back(callback);
  if (callbacks.size() > 1) {
    BLOG(1, "Fetch already in progress for publisher " << publisher_key);
    return;
  }

  BLOG(1, "Fetching server publisher info for " << publisher_key);

  std::string hex_prefix = GetHashPrefixInHex(
      publisher_key,
      kQueryPrefixBytes);

  // Due to privacy concerns, the request length must be consistent
  // for all publisher lookups. Do not add URL parameters or headers
  // whose size will vary depending on the publisher key.
  std::string url = braveledger_request_util::GetPublisherInfoUrl(hex_prefix);
  ledger_->LoadURL(
      url, {}, "", "",
      ledger::UrlMethod::GET,
      std::bind(&ServerPublisherFetcher::OnFetchCompleted,
          this, publisher_key, _1));
}

void ServerPublisherFetcher::OnFetchCompleted(
    const std::string& publisher_key,
    const ledger::UrlResponse& response) {
  BLOG(6, ledger::UrlResponseToString(__func__, response));
  auto server_info = braveledger_response_util::ParsePublisherInfo(
      publisher_key,
      response.status_code,
      response.body);

  if (!server_info) {
    RunCallbacks(publisher_key, nullptr);
    return;
  }

  // Create a shared pointer to a mojo struct so that it can be copied
  // into a callback.
  auto shared_info = std::make_shared<ledger::ServerPublisherInfoPtr>(
      std::move(server_info));

  // Store the result for subsequent lookups.
  ledger_->database()->InsertServerPublisherInfo(**shared_info,
      [this, publisher_key, shared_info](ledger::Result result) {
        if (result != ledger::Result::LEDGER_OK) {
          BLOG(0, "Error saving server publisher info record");
        }
        RunCallbacks(publisher_key, std::move(*shared_info));
      });
}

bool ServerPublisherFetcher::IsExpired(
    ledger::ServerPublisherInfo* server_info) {
  if (!server_info) {
    return true;
  }

  auto last_update_time = base::Time::FromDoubleT(server_info->updated_at);
  auto age = base::Time::Now() - last_update_time;

  if (age.InSeconds() < 0) {
    // A negative age value indicates that either the data is
    // corrupted or that we are incorrectly storing the timestamp.
    // Pessimistically assume that we are incorrectly storing
    // the timestamp in order to avoid a case where we fetch
    // on every tab update.
    BLOG(0, "Server publisher info has a future updated_at time.");
  }

  return age.InSeconds() > GetCacheExpiryInSeconds(ledger_);
}

void ServerPublisherFetcher::PurgeExpiredRecords() {
  BLOG(1, "Purging expired server publisher info records");
  int64_t max_age = GetCacheExpiryInSeconds(ledger_) * 2;
  ledger_->database()->DeleteExpiredServerPublisherInfo(
      max_age,
      [](auto result) {});
}

FetchCallbackVector ServerPublisherFetcher::GetCallbacks(
    const std::string& publisher_key) {
  FetchCallbackVector callbacks;
  auto iter = callback_map_.find(publisher_key);
  if (iter != callback_map_.end()) {
    callbacks = std::move(iter->second);
    callback_map_.erase(iter);
  }
  return callbacks;
}

void ServerPublisherFetcher::RunCallbacks(
    const std::string& publisher_key,
    ledger::ServerPublisherInfoPtr server_info) {
  FetchCallbackVector callbacks = GetCallbacks(publisher_key);
  DCHECK(!callbacks.empty());
  for (auto& callback : callbacks) {
    callback(server_info ? server_info.Clone() : nullptr);
  }
}

}  // namespace braveledger_publisher
