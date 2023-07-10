/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_SERVER_PUBLISHER_FETCHER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_SERVER_PUBLISHER_FETCHER_H_

#include <map>
#include <string>
#include <vector>

#include "base/functional/callback.h"
#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/endpoint/private_cdn/private_cdn_server.h"
#include "brave/components/brave_rewards/core/rewards_engine_helper.h"

namespace brave_rewards::internal {

// Fetches server publisher info and provides methods for determining whether a
// server publisher info record is expired.
class ServerPublisherFetcher : public RewardsEngineHelper {
 public:
  ~ServerPublisherFetcher() override;

  // Returns a value indicating whether a server info record with the specified
  // last update time is expired.
  bool IsExpired(const mojom::ServerPublisherInfo& server_info);

  using FetchCallback = base::OnceCallback<void(mojom::ServerPublisherInfoPtr)>;

  // Fetches server publisher info for the specified publisher key.
  void Fetch(const std::string& publisher_key, FetchCallback callback);

  // Purges expired records from the backing database.
  void PurgeExpiredRecords();

 private:
  friend RewardsEngineContext;

  inline static const char kUserDataKey[] = "server-publisher-fetcher";

  explicit ServerPublisherFetcher(RewardsEngineContext& context);

  void OnFetchCompleted(const std::string& publisher_key,
                        mojom::Result result,
                        mojom::ServerPublisherInfoPtr info);

  void OnInfoSaved(const std::string& publisher_key,
                   mojom::ServerPublisherInfoPtr info,
                   mojom::Result result);

  std::vector<FetchCallback> GetCallbacks(const std::string& publisher_key);

  void RunCallbacks(const std::string& publisher_key,
                    mojom::ServerPublisherInfoPtr server_info);

  std::map<std::string, std::vector<FetchCallback>> callback_map_;
  endpoint::PrivateCDNServer private_cdn_server_;
  base::WeakPtrFactory<ServerPublisherFetcher> weak_factory_{this};
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_PUBLISHER_SERVER_PUBLISHER_FETCHER_H_
