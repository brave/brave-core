/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_MEDIA_PUBLISHER_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_MEDIA_PUBLISHER_INFO_H_

#include <string>

#include "brave/components/brave_rewards/core/database/database_table.h"

namespace brave_rewards::internal {
namespace database {

class DatabaseMediaPublisherInfo : public DatabaseTable {
 public:
  explicit DatabaseMediaPublisherInfo(RewardsEngineImpl& engine);
  ~DatabaseMediaPublisherInfo() override;

  void InsertOrUpdate(const std::string& media_key,
                      const std::string& publisher_key,
                      LegacyResultCallback callback);

  void GetRecord(const std::string& media_key, PublisherInfoCallback callback);

 private:
  void OnGetRecord(PublisherInfoCallback callback,
                   mojom::DBCommandResponsePtr response);
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_MEDIA_PUBLISHER_INFO_H_
