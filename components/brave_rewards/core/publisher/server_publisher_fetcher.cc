/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/publisher/server_publisher_fetcher.h"

#include <utility>

#include "base/time/time.h"
#include "brave/components/brave_rewards/core/common/legacy_callback_helpers.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/publisher/prefix_util.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"

namespace brave_rewards::internal {

namespace {

constexpr size_t kQueryPrefixBytes = 2;

int64_t GetCacheExpiryInSeconds() {
  // NOTE: We are reusing the publisher prefix list refresh interval for
  // determining the cache lifetime of publisher details. At a later
  // time we may want to introduce an additional option for this value.
  return kPublisherListRefreshInterval;
}

}  // namespace

ServerPublisherFetcher::ServerPublisherFetcher(RewardsEngineContext& context)
    : RewardsEngineHelper(context),
      private_cdn_server_(context.GetEngineImpl()) {}

ServerPublisherFetcher::~ServerPublisherFetcher() = default;

void ServerPublisherFetcher::Fetch(const std::string& publisher_key,
                                   FetchCallback callback) {
  auto& callbacks = callback_map_[publisher_key];
  callbacks.push_back(std::move(callback));
  if (callbacks.size() > 1) {
    Log(FROM_HERE) << "Fetch already in progress";
    return;
  }

  const std::string hex_prefix =
      GetPublisherHashPrefixInHex(publisher_key, kQueryPrefixBytes);

  private_cdn_server_.get_publisher().Request(
      publisher_key, hex_prefix,
      ToLegacyCallback(base::BindOnce(&ServerPublisherFetcher::OnFetchCompleted,
                                      weak_factory_.GetWeakPtr(),
                                      publisher_key)));
}

void ServerPublisherFetcher::OnFetchCompleted(
    const std::string& publisher_key,
    mojom::Result result,
    mojom::ServerPublisherInfoPtr info) {
  if (result != mojom::Result::OK) {
    RunCallbacks(publisher_key, nullptr);
    return;
  }

  // Store the result for subsequent lookups.
  context().GetEngineImpl().database()->InsertServerPublisherInfo(
      *info, ToLegacyCallback(base::BindOnce(
                 &ServerPublisherFetcher::OnInfoSaved,
                 weak_factory_.GetWeakPtr(), publisher_key, info.Clone())));
}

void ServerPublisherFetcher::OnInfoSaved(const std::string& publisher_key,
                                         mojom::ServerPublisherInfoPtr info,
                                         mojom::Result result) {
  if (result != mojom::Result::OK) {
    LogError(FROM_HERE) << "Error saving server publisher info record";
  }
  RunCallbacks(publisher_key, std::move(info));
}

bool ServerPublisherFetcher::IsExpired(
    const mojom::ServerPublisherInfo& server_info) {
  auto last_update_time = base::Time::FromDoubleT(server_info.updated_at);
  auto age = base::Time::Now() - last_update_time;

  if (age.InSeconds() < 0) {
    // A negative age value indicates that either the data is corrupted or that
    // we are incorrectly storing the timestamp. Pessimistically assume that we
    // are incorrectly storing the timestamp in order to avoid a case where we
    // fetch on every tab update.
    LogError(FROM_HERE) << "Server publisher info has a future updated_at time";
  }

  return age.InSeconds() > GetCacheExpiryInSeconds();
}

void ServerPublisherFetcher::PurgeExpiredRecords() {
  LogError(FROM_HERE) << "Purging expired server publisher info records";
  int64_t max_age = GetCacheExpiryInSeconds() * 2;
  context().GetEngineImpl().database()->DeleteExpiredServerPublisherInfo(
      max_age, [](auto result) {});
}

std::vector<ServerPublisherFetcher::FetchCallback>
ServerPublisherFetcher::GetCallbacks(const std::string& publisher_key) {
  std::vector<ServerPublisherFetcher::FetchCallback> callbacks;
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
  auto callbacks = GetCallbacks(publisher_key);
  DCHECK(!callbacks.empty());
  for (auto& callback : callbacks) {
    std::move(callback).Run(server_info ? server_info.Clone() : nullptr);
  }
  client().OnPublisherUpdated(publisher_key);
}

}  // namespace brave_rewards::internal
