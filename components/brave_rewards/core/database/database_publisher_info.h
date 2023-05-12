/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PUBLISHER_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PUBLISHER_INFO_H_

#include <string>

#include "brave/components/brave_rewards/core/database/database_table.h"

namespace brave_rewards::internal::database {

class DatabasePublisherInfo {
 public:
  void InsertOrUpdate(mojom::PublisherInfoPtr info,
                      LegacyResultCallback callback);

  void GetRecord(const std::string& publisher_key,
                 GetPublisherInfoCallback callback);

  void GetPanelRecord(mojom::ActivityInfoFilterPtr filter,
                      GetPublisherPanelInfoCallback callback);

  void RestorePublishers(ResultCallback callback);

  void GetExcludedList(GetExcludedListCallback callback);

 private:
  void OnGetRecord(mojom::DBCommandResponsePtr response,
                   GetPublisherInfoCallback callback);

  void OnGetPanelRecord(mojom::DBCommandResponsePtr response,
                        GetPublisherPanelInfoCallback callback);

  void OnRestorePublishers(ResultCallback callback,
                           mojom::DBCommandResponsePtr response);

  void OnGetExcludedList(mojom::DBCommandResponsePtr response,
                         GetExcludedListCallback callback);
};

}  // namespace brave_rewards::internal::database

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PUBLISHER_INFO_H_
