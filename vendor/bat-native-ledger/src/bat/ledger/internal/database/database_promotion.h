/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_PROMOTION_H_
#define BRAVELEDGER_DATABASE_DATABASE_PROMOTION_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_table.h"

namespace ledger {
namespace database {

using GetPromotionCallback = std::function<void(type::PromotionPtr)>;

class DatabasePromotion: public DatabaseTable {
 public:
  explicit DatabasePromotion(LedgerImpl* ledger);
  ~DatabasePromotion() override;

  void InsertOrUpdate(
      type::PromotionPtr info,
      ledger::ResultCallback callback);

  void GetRecord(
      const std::string& id,
      GetPromotionCallback callback);

  void GetRecords(
      const std::vector<std::string>& ids,
      client::GetPromotionListCallback callback);

  void GetAllRecords(
      ledger::GetAllPromotionsCallback callback);

  void SaveClaimId(
      const std::string& promotion_id,
      const std::string& claim_id,
      ledger::ResultCallback callback);

  void UpdateStatus(
      const std::string& promotion_id,
      const type::PromotionStatus status,
      ledger::ResultCallback callback);

  void UpdateRecordsStatus(
      const std::vector<std::string>& ids,
      const type::PromotionStatus status,
      ledger::ResultCallback callback);

  void CredentialCompleted(
      const std::string& promotion_id,
      ledger::ResultCallback callback);

  void GetRecordsByType(
      const std::vector<type::PromotionType>& types,
      client::GetPromotionListCallback callback);

  void UpdateRecordsBlankPublicKey(
      const std::vector<std::string>& ids,
      ledger::ResultCallback callback);

 private:
  void OnGetRecord(
      type::DBCommandResponsePtr response,
      GetPromotionCallback callback);

  void OnGetAllRecords(
      type::DBCommandResponsePtr response,
      ledger::GetAllPromotionsCallback callback);

  void OnGetRecords(
      type::DBCommandResponsePtr response,
      client::GetPromotionListCallback callback);
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVELEDGER_DATABASE_DATABASE_PROMOTION_H_
