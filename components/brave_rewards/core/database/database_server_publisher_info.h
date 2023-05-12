/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_SERVER_PUBLISHER_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_SERVER_PUBLISHER_INFO_H_

#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/database/database_server_publisher_banner.h"
#include "brave/components/brave_rewards/core/database/database_table.h"

namespace brave_rewards::internal::database {

using GetServerPublisherInfoCallback =
    std::function<void(mojom::ServerPublisherInfoPtr)>;

class DatabaseServerPublisherInfo {
 public:
  void InsertOrUpdate(const mojom::ServerPublisherInfo& server_info,
                      LegacyResultCallback callback);

  void GetRecord(const std::string& publisher_key,
                 GetServerPublisherInfoCallback callback);

  void DeleteExpiredRecords(int64_t max_age_seconds,
                            LegacyResultCallback callback);

 private:
  void OnGetRecordBanner(mojom::PublisherBannerPtr banner,
                         const std::string& publisher_key,
                         GetServerPublisherInfoCallback callback);

  void OnGetRecord(mojom::DBCommandResponsePtr response,
                   const std::string& publisher_key,
                   const mojom::PublisherBanner& banner,
                   GetServerPublisherInfoCallback callback);

  void OnExpiredRecordsSelected(mojom::DBCommandResponsePtr response,
                                LegacyResultCallback callback);

  DatabaseServerPublisherBanner banner_;
};

}  // namespace brave_rewards::internal::database

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_SERVER_PUBLISHER_INFO_H_
