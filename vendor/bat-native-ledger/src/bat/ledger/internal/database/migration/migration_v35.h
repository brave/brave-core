/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V35_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V35_H_

namespace ledger {
namespace database {
namespace migration {

const char v35[] = R"(
  CREATE TABLE external_transactions (
    wallet_provider INTEGER NOT NULL CHECK(wallet_provider >= 0 AND wallet_provider <= 2),
    transaction_id TEXT NOT NULL CHECK(transaction_id <> ''),
    contribution_id TEXT NOT NULL CHECK(contribution_id <> ''),
    is_fee INTEGER NOT NULL CHECK(is_fee = 0 OR is_fee == 1),
    status INTEGER NOT NULL CHECK(status >= 0 AND status <= 2),
    created_at TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,

    PRIMARY KEY (wallet_provider, transaction_id),
    FOREIGN KEY (contribution_id) REFERENCES contribution_info (contribution_id) ON UPDATE RESTRICT ON DELETE RESTRICT,
    UNIQUE(contribution_id, is_fee)
  );
  
  CREATE INDEX external_transactions_contribution_id_is_fee_index ON external_transactions (contribution_id, is_fee);
)";

}  // namespace migration
}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V35_H_
