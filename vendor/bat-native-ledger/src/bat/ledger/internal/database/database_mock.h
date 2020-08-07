/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_DATABASE_DATABASE_MOCK_H_
#define BAT_LEDGER_DATABASE_DATABASE_MOCK_H_

#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/database/database.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace braveledger_database {

class MockDatabase : public Database {
 public:
  explicit MockDatabase(bat_ledger::LedgerImpl* ledger);

  ~MockDatabase() override;

  MOCK_METHOD2(GetContributionInfo, void(
      const std::string& contribution_id,
      ledger::GetContributionInfoCallback callback));

  MOCK_METHOD2(GetReservedUnblindedTokens, void(
      const std::string& redeem_id,
      ledger::GetUnblindedTokenListCallback callback));

  MOCK_METHOD2(SavePromotion, void(
      ledger::PromotionPtr info,
      ledger::ResultCallback callback));

  MOCK_METHOD1(GetAllPromotions,
      void(ledger::GetAllPromotionsCallback callback));
};

}  // namespace braveledger_database

#endif  // BAT_LEDGER_DATABASE_DATABASE_MOCK_H_
