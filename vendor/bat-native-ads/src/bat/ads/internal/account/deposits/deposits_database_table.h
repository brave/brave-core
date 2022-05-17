/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_DEPOSITS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_DEPOSITS_DATABASE_TABLE_H_

#include <string>

#include "bat/ads/ads_client_aliases.h"
#include "bat/ads/internal/account/deposits/deposits_database_table_aliases.h"
#include "bat/ads/internal/ad_server/catalog/bundle/creative_ad_info_aliases.h"
#include "bat/ads/internal/database/database_table_interface.h"
#include "bat/ads/public/interfaces/ads.mojom.h"

namespace ads {

namespace database {
namespace table {

class Deposits final : public TableInterface {
 public:
  Deposits();
  ~Deposits() override;

  void Save(const DepositInfo& deposit, ResultCallback callback);

  void InsertOrUpdate(mojom::DBTransaction* transaction,
                      const CreativeAdList& creative_ads);
  void InsertOrUpdate(mojom::DBTransaction* transaction,
                      const DepositInfo& deposit);

  void GetForCreativeInstanceId(const std::string& creative_instance_id,
                                GetDepositsCallback callback);

  void PurgeExpired(ResultCallback callback);

  std::string GetTableName() const override;

  void Migrate(mojom::DBTransaction* transaction,
               const int to_version) override;

 private:
  std::string BuildInsertOrUpdateQuery(mojom::DBCommand* command,
                                       const CreativeAdList& creative_ads);
  std::string BuildInsertOrUpdateQuery(mojom::DBCommand* command,
                                       const DepositInfo& deposit);

  void OnGetForCreativeInstanceId(mojom::DBCommandResponsePtr response,
                                  const std::string& creative_instance_id,
                                  GetDepositsCallback callback);

  void MigrateToV24(mojom::DBTransaction* transaction);
};

}  // namespace table
}  // namespace database
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_DEPOSITS_DATABASE_TABLE_H_
