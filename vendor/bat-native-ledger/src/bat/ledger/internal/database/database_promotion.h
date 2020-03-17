/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_PROMOTION_H_
#define BRAVELEDGER_DATABASE_DATABASE_PROMOTION_H_

#include <memory>
#include <string>
#include <vector>

#include "bat/ledger/internal/database/database_promotion_creds.h"
#include "bat/ledger/internal/database/database_table.h"

namespace braveledger_database {

class DatabasePromotion: public DatabaseTable {
 public:
  explicit DatabasePromotion(bat_ledger::LedgerImpl* ledger);
  ~DatabasePromotion() override;

  bool Migrate(ledger::DBTransaction* transaction, const int target) override;

  void InsertOrUpdate(
      ledger::PromotionPtr info,
      ledger::ResultCallback callback);

  void GetRecord(
      const std::string& id,
      ledger::GetPromotionCallback callback);

  void GetRecords(
      const std::vector<std::string>& ids,
      ledger::GetPromotionListCallback callback);

  void GetAllRecords(
      ledger::GetAllPromotionsCallback callback);

  void DeleteRecordList(
      const std::vector<std::string>& ids,
      ledger::ResultCallback callback);

  void SaveClaimId(
      const std::string& promotion_id,
      const std::string& claim_id,
      ledger::ResultCallback callback);

  void UpdateStatus(
      const std::string& promotion_id,
      const ledger::PromotionStatus status,
      ledger::ResultCallback callback);

  void CredentialCompleted(
      const std::string& promotion_id,
      ledger::ResultCallback callback);

 private:
  bool CreateTableV10(ledger::DBTransaction* transaction);

  bool CreateIndexV10(ledger::DBTransaction* transaction);

  bool MigrateToV10(ledger::DBTransaction* transaction);

  bool MigrateToV13(ledger::DBTransaction* transaction);

  bool MigrateToV14(ledger::DBTransaction* transaction);

  bool MigrateToV15(ledger::DBTransaction* transaction);

  bool MigrateToV18(ledger::DBTransaction* transaction);

  void OnGetRecord(
      ledger::DBCommandResponsePtr response,
      ledger::GetPromotionCallback callback);

  void OnGetAllRecords(
      ledger::DBCommandResponsePtr response,
      ledger::GetAllPromotionsCallback callback);

  void OnGetRecords(
      ledger::DBCommandResponsePtr response,
      ledger::GetPromotionListCallback callback);

  std::unique_ptr<DatabasePromotionCreds> creds_;
};

}  // namespace braveledger_database

#endif  // BRAVELEDGER_DATABASE_DATABASE_PROMOTION_H_
