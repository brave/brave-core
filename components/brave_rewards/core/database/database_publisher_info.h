/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PUBLISHER_INFO_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PUBLISHER_INFO_H_

#include <string>

#include "brave/components/brave_rewards/core/database/database_table.h"

namespace brave_rewards::core {
namespace database {

class DatabasePublisherInfo : public DatabaseTable {
 public:
  explicit DatabasePublisherInfo(LedgerImpl* ledger);
  ~DatabasePublisherInfo() override;

  void InsertOrUpdate(mojom::PublisherInfoPtr info,
                      LegacyResultCallback callback);

  void GetRecord(const std::string& publisher_key,
                 PublisherInfoCallback callback);

  void GetPanelRecord(mojom::ActivityInfoFilterPtr filter,
                      PublisherInfoCallback callback);

  void RestorePublishers(ResultCallback callback);

  void GetExcludedList(PublisherInfoListCallback callback);

 private:
  void OnGetRecord(mojom::DBCommandResponsePtr response,
                   PublisherInfoCallback callback);

  void OnGetPanelRecord(mojom::DBCommandResponsePtr response,
                        PublisherInfoCallback callback);

  void OnRestorePublishers(ResultCallback callback,
                           mojom::DBCommandResponsePtr response);

  void OnGetExcludedList(mojom::DBCommandResponsePtr response,
                         PublisherInfoListCallback callback);
};

}  // namespace database
}  // namespace brave_rewards::core

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_DATABASE_DATABASE_PUBLISHER_INFO_H_
