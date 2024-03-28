/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_SERVER_PUBLISHER_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_SERVER_PUBLISHER_FETCHER_H_

#include <map>
#include <string>
#include <vector>

#include "base/memory/raw_ref.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/database/database_server_publisher_info.h"
#include "brave/components/brave_rewards/core/endpoint/private_cdn/private_cdn_server.h"
#include "brave/components/brave_rewards/core/rewards_callbacks.h"

namespace brave_rewards::internal {
class RewardsEngine;

namespace publisher {

// Fetches server publisher info and provides methods for determining
// whether a server publisher info record is expired
class ServerPublisherFetcher {
 public:
  explicit ServerPublisherFetcher(RewardsEngine& engine);

  ServerPublisherFetcher(const ServerPublisherFetcher&) = delete;
  ServerPublisherFetcher& operator=(const ServerPublisherFetcher&) = delete;

  ~ServerPublisherFetcher();

  // Returns a value indicating whether a server info record with
  // the specified last update time is expired
  bool IsExpired(mojom::ServerPublisherInfo* server_info);

  using FetchCallback = base::OnceCallback<void(mojom::ServerPublisherInfoPtr)>;

  // Fetches server publisher info for the specified publisher key
  void Fetch(const std::string& publisher_key, FetchCallback callback);

  // Purges expired records from the backing database
  void PurgeExpiredRecords();

 private:
  void OnFetchCompleted(const std::string& publisher_key,
                        mojom::Result result,
                        mojom::ServerPublisherInfoPtr info);

  void OnRecordSaved(const std::string& publisher_key,
                     mojom::ServerPublisherInfoPtr info,
                     mojom::Result result);

  using FetchCallbackVector = std::vector<FetchCallback>;

  FetchCallbackVector GetCallbacks(const std::string& publisher_key);

  void RunCallbacks(const std::string& publisher_key,
                    mojom::ServerPublisherInfoPtr server_info);

  const raw_ref<RewardsEngine> engine_;
  std::map<std::string, FetchCallbackVector> callback_map_;
  endpoint::PrivateCDNServer private_cdn_server_;
  base::WeakPtrFactory<ServerPublisherFetcher> weak_factory_{this};
};

}  // namespace publisher
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_SERVER_PUBLISHER_FETCHER_H_
