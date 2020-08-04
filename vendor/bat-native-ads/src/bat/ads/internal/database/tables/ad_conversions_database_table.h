/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_DATABASE_AD_CONVERSIONS_DATABASE_TABLE_H_
#define BAT_ADS_INTERNAL_DATABASE_AD_CONVERSIONS_DATABASE_TABLE_H_

#include <string>

#include "bat/ads/ads_client.h"
#include "bat/ads/internal/ad_conversions/ad_conversion_info.h"
#include "bat/ads/internal/database/database_table.h"
#include "bat/ads/mojom.h"
#include "bat/ads/result.h"

namespace ads {

using GetAdConversionsCallback = std::function<void(const Result,
    const AdConversionList&)>;

class AdsImpl;

namespace database {
namespace table {

class AdConversions : public Table {
 public:
  explicit AdConversions(
      AdsImpl* ads);

  ~AdConversions() override;

  void Save(
      const AdConversionList& ad_conversions,
      ResultCallback callback);

  void GetAdConversions(
      GetAdConversionsCallback callback);

  void PurgeExpiredAdConversions(
      ResultCallback callback);

  std::string get_table_name() const override;

  void Migrate(
      DBTransaction* transaction,
      const int to_version) override;

 private:
  void InsertOrUpdate(
      DBTransaction* transaction,
      const AdConversionList& ad_conversion);

  int BindParameters(
      DBCommand* command,
      const AdConversionList& ad_conversion);

  std::string BuildInsertOrUpdateQuery(
      DBCommand* command,
      const AdConversionList& ad_conversions);

  void OnGetAdConversions(
      DBCommandResponsePtr response,
      GetAdConversionsCallback callback);

  AdConversionInfo GetAdConversionFromRecord(
      DBRecord* record) const;

  void CreateTableV1(
      DBTransaction* transaction);
  void CreateIndexV1(
      DBTransaction* transaction);
  void MigrateToV1(
      DBTransaction* transaction);

  AdsImpl* ads_;  // NOT OWNED
};

}  // namespace table
}  // namespace database
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_DATABASE_AD_CONVERSIONS_DATABASE_TABLE_H_
