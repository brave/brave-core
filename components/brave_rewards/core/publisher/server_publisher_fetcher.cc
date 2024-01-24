/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/publisher/server_publisher_fetcher.h"

#include <memory>
#include <utility>

#include "base/big_endian.h"
#include "base/json/json_reader.h"
#include "base/strings/stringprintf.h"
#include "base/time/time.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/publisher/prefix_util.h"
#include "brave/components/brave_rewards/core/publisher/protos/channel_response.pb.h"
#include "brave/components/brave_rewards/core/publisher/publisher_prefix_list_updater.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave_base/random.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

namespace brave_rewards::internal::publisher {

namespace {

constexpr size_t kQueryPrefixBytes = 2;

int64_t GetCacheExpiryInSeconds() {
  // NOTE: We are reusing the publisher prefix list refresh interval for
  // determining the cache lifetime of publisher details. At a later
  // time we may want to introduce an additional option for this value.
  return PublisherPrefixListUpdater::kRefreshInterval;
}

}  // namespace

ServerPublisherFetcher::ServerPublisherFetcher(RewardsEngineImpl& engine)
    : engine_(engine), private_cdn_server_(engine) {}

ServerPublisherFetcher::~ServerPublisherFetcher() = default;

void ServerPublisherFetcher::Fetch(
    const std::string& publisher_key,
    database::GetServerPublisherInfoCallback callback) {
  FetchCallbackVector& callbacks = callback_map_[publisher_key];
  callbacks.push_back(callback);
  if (callbacks.size() > 1) {
    BLOG(1, "Fetch already in progress");
    return;
  }

  const std::string hex_prefix =
      GetHashPrefixInHex(publisher_key, kQueryPrefixBytes);

  auto url_callback = std::bind(&ServerPublisherFetcher::OnFetchCompleted, this,
                                _1, _2, publisher_key);

  private_cdn_server_.get_publisher().Request(publisher_key, hex_prefix,
                                              url_callback);
}

void ServerPublisherFetcher::OnFetchCompleted(
    const mojom::Result result,
    mojom::ServerPublisherInfoPtr info,
    const std::string& publisher_key) {
  if (result != mojom::Result::OK) {
    RunCallbacks(publisher_key, nullptr);
    return;
  }

  // Create a shared pointer to a mojo struct so that it can be copied
  // into a callback.
  auto shared_info =
      std::make_shared<mojom::ServerPublisherInfoPtr>(std::move(info));

  // Store the result for subsequent lookups.
  engine_->database()->InsertServerPublisherInfo(
      **shared_info, [this, publisher_key, shared_info](mojom::Result result) {
        if (result != mojom::Result::OK) {
          BLOG(0, "Error saving server publisher info record");
        }
        RunCallbacks(publisher_key, std::move(*shared_info));
      });
}

bool ServerPublisherFetcher::IsExpired(
    mojom::ServerPublisherInfo* server_info) {
  if (!server_info) {
    return true;
  }

  auto last_update_time =
      base::Time::FromSecondsSinceUnixEpoch(server_info->updated_at);
  auto age = base::Time::Now() - last_update_time;

  if (age.InSeconds() < 0) {
    // A negative age value indicates that either the data is
    // corrupted or that we are incorrectly storing the timestamp.
    // Pessimistically assume that we are incorrectly storing
    // the timestamp in order to avoid a case where we fetch
    // on every tab update.
    BLOG(0, "Server publisher info has a future updated_at time.");
  }

  return age.InSeconds() > GetCacheExpiryInSeconds();
}

void ServerPublisherFetcher::PurgeExpiredRecords() {
  BLOG(1, "Purging expired server publisher info records");
  int64_t max_age = GetCacheExpiryInSeconds() * 2;
  engine_->database()->DeleteExpiredServerPublisherInfo(max_age,
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
    mojom::ServerPublisherInfoPtr server_info) {
  FetchCallbackVector callbacks = GetCallbacks(publisher_key);
  DCHECK(!callbacks.empty());
  for (auto& callback : callbacks) {
    callback(server_info ? server_info.Clone() : nullptr);
  }
  engine_->client()->OnPublisherUpdated(publisher_key);
}

}  // namespace brave_rewards::internal::publisher
