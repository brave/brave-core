/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PUBLISHER_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PUBLISHER_INFO_H_

#include <string>

#include "brave/components/brave_rewards/core/database/database_table.h"

namespace brave_rewards::internal {
namespace database {

class DatabasePublisherInfo : public DatabaseTable {
 public:
  explicit DatabasePublisherInfo(RewardsEngineImpl& engine);
  ~DatabasePublisherInfo() override;

  void InsertOrUpdate(mojom::PublisherInfoPtr info,
                      LegacyResultCallback callback);

  void GetRecord(const std::string& publisher_key,
                 GetPublisherInfoCallback callback);

  void GetPanelRecord(mojom::ActivityInfoFilterPtr filter,
                      GetPublisherPanelInfoCallback callback);

  void RestorePublishers(ResultCallback callback);

  void GetExcludedList(GetExcludedListCallback callback);

 private:
  void OnGetRecord(GetPublisherInfoCallback callback,
                   mojom::DBCommandResponsePtr response);

  void OnGetPanelRecord(GetPublisherPanelInfoCallback callback,
                        mojom::DBCommandResponsePtr response);

  void OnRestorePublishers(ResultCallback callback,
                           mojom::DBCommandResponsePtr response);

  void OnGetExcludedList(GetExcludedListCallback callback,
                         mojom::DBCommandResponsePtr response);
};

}  // namespace database
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PUBLISHER_INFO_H_
