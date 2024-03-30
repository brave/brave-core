/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_SERVER_PUBLISHER_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_SERVER_PUBLISHER_INFO_H_

#include <string>
#include <vector>

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_rewards/core/database/database_server_publisher_banner.h"
#include "brave/components/brave_rewards/core/database/database_table.h"

namespace brave_rewards::internal {
namespace database {

class DatabaseServerPublisherInfo : public DatabaseTable {
 public:
  explicit DatabaseServerPublisherInfo(RewardsEngine& engine);
  ~DatabaseServerPublisherInfo() override;

  void InsertOrUpdate(const mojom::ServerPublisherInfo& server_info,
                      ResultCallback callback);

  void GetRecord(const std::string& publisher_key,
                 GetServerPublisherInfoCallback callback);

  void DeleteExpiredRecords(int64_t max_age_seconds, ResultCallback callback);

 private:
  void OnGetRecordBanner(const std::string& publisher_key,
                         GetServerPublisherInfoCallback callback,
                         mojom::PublisherBannerPtr banner);

  void OnGetRecord(GetServerPublisherInfoCallback callback,
                   const std::string& publisher_key,
                   mojom::PublisherBannerPtr banner,
                   mojom::DBCommandResponsePtr response);

  void OnExpiredRecordsSelected(ResultCallback callback,
                                mojom::DBCommandResponsePtr response);

  DatabaseServerPublisherBanner banner_;
  base::WeakPtrFactory<DatabaseServerPublisherInfo> weak_factory_{this};
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_SERVER_PUBLISHER_INFO_H_
