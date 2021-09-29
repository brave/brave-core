/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V20_H_
#define BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V20_H_

namespace ledger {
namespace database {
namespace migration {

const char v20[] = R"(
  DROP INDEX IF EXISTS unblinded_tokens_creds_id_index;

  ALTER TABLE unblinded_tokens ADD redeemed_at TIMESTAMP NOT NULL DEFAULT 0;

  ALTER TABLE unblinded_tokens ADD redeem_id TEXT;

  ALTER TABLE unblinded_tokens ADD redeem_type INTEGER NOT NULL DEFAULT 0;

  CREATE INDEX unblinded_tokens_creds_id_index ON unblinded_tokens (creds_id);

  CREATE INDEX unblinded_tokens_redeem_id_index ON unblinded_tokens (redeem_id);
)";

}  // namespace migration
}  // namespace database
}  // namespace ledger

#endif  // BRAVE_VENDOR_BAT_NATIVE_LEDGER_SRC_BAT_LEDGER_INTERNAL_DATABASE_MIGRATION_MIGRATION_V20_H_
