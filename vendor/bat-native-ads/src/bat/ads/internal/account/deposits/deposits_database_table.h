/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_DEPOSITS_DATABASE_TABLE_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_DEPOSITS_DATABASE_TABLE_H_

#include <functional>
#include <string>

#include "absl/types/optional.h"
#include "bat/ads/ads_client_callback.h"
#include "bat/ads/internal/account/deposits/deposit_info.h"
#include "bat/ads/internal/creatives/creative_ad_info.h"
#include "bat/ads/internal/database/database_table_interface.h"
#include "bat/ads/public/interfaces/ads.mojom-forward.h"

namespace ads {
namespace database {
namespace table {

using GetDepositsCallback =
    std::function<void(const bool success,
                       const absl::optional<DepositInfo>& deposit)>;

class Deposits final : public TableInterface {
 public:
  Deposits();
  ~Deposits() override;
  Deposits(const Deposits&) = delete;
  Deposits& operator=(const Deposits&) = delete;

  void Save(const DepositInfo& deposit, ResultCallback callback);

  void InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                      const CreativeAdList& creative_ads);
  void InsertOrUpdate(mojom::DBTransactionInfo* transaction,
                      const DepositInfo& deposit);

  void GetForCreativeInstanceId(const std::string& creative_instance_id,
                                GetDepositsCallback callback);

  void PurgeExpired(ResultCallback callback) const;

  std::string GetTableName() const override;

  void Migrate(mojom::DBTransactionInfo* transaction, int to_version) override;

 private:
  std::string BuildInsertOrUpdateQuery(
      mojom::DBCommandInfo* command,
      const CreativeAdList& creative_ads) const;
  std::string BuildInsertOrUpdateQuery(mojom::DBCommandInfo* command,
                                       const DepositInfo& deposit) const;

  void OnGetForCreativeInstanceId(const std::string& creative_instance_id,
                                  GetDepositsCallback callback,
                                  mojom::DBCommandResponseInfoPtr response);

  void MigrateToV24(mojom::DBTransactionInfo* transaction);
};

}  // namespace table
}  // namespace database
}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_DEPOSITS_DEPOSITS_DATABASE_TABLE_H_
