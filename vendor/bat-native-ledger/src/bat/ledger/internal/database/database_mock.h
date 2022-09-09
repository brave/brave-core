/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_MOCK_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_MOCK_H_

#include <string>

#include "bat/ledger/ledger.h"
#include "bat/ledger/internal/database/database.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace ledger {
namespace database {

class MockDatabase : public Database {
 public:
  explicit MockDatabase(LedgerImpl* ledger);

  ~MockDatabase() override;

  MOCK_METHOD2(GetContributionInfo, void(
      const std::string& contribution_id,
      GetContributionInfoCallback callback));

  MOCK_METHOD2(GetReservedUnblindedTokens, void(
      const std::string& redeem_id,
      GetUnblindedTokenListCallback callback));

  MOCK_METHOD2(SavePromotion,
               void(mojom::PromotionPtr info,
                    ledger::LegacyResultCallback callback));

  MOCK_METHOD1(GetAllPromotions,
      void(ledger::GetAllPromotionsCallback callback));
};

}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_DATABASE_MOCK_H_
