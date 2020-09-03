/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_LEDGER_LEDGER_DATABASE_H_
#define BAT_LEDGER_LEDGER_DATABASE_H_

#include <string>

#include "base/files/file_path.h"
#include "bat/ledger/ledger_client.h"

namespace ledger {

class LEDGER_EXPORT LedgerDatabase {
 public:
  LedgerDatabase() = default;
  virtual ~LedgerDatabase() = default;

  LedgerDatabase(const LedgerDatabase&) = delete;
  LedgerDatabase& operator=(const LedgerDatabase&) = delete;

  static LedgerDatabase* CreateInstance(const base::FilePath& path);

  virtual void RunTransaction(
      type::DBTransactionPtr transaction,
      type::DBCommandResponse* command_response) = 0;
};

}  // namespace ledger

#endif  // BAT_LEDGER_LEDGER_DATABASE_H_
