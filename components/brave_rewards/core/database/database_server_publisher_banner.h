/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_SERVER_PUBLISHER_BANNER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_SERVER_PUBLISHER_BANNER_H_

#include <map>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/database/database_server_publisher_links.h"
#include "brave/components/brave_rewards/core/database/database_table.h"

namespace brave_rewards::internal {
namespace database {

class DatabaseServerPublisherBanner : public DatabaseTable {
 public:
  explicit DatabaseServerPublisherBanner(RewardsEngineImpl& engine);
  ~DatabaseServerPublisherBanner() override;

  void InsertOrUpdate(mojom::DBTransaction* transaction,
                      const mojom::ServerPublisherInfo& server_info);

  void DeleteRecords(mojom::DBTransaction* transaction,
                     const std::string& publisher_key_list);

  void GetRecord(const std::string& publisher_key,
                 GetPublisherBannerCallback callback);

 private:
  void OnGetRecord(GetPublisherBannerCallback callback,
                   const std::string& publisher_key,
                   mojom::DBCommandResponsePtr response);

  void OnGetRecordLinks(const std::map<std::string, std::string>& links,
                        const mojom::PublisherBanner& banner,
                        GetPublisherBannerCallback callback);

  DatabaseServerPublisherLinks links_;
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_SERVER_PUBLISHER_BANNER_H_
