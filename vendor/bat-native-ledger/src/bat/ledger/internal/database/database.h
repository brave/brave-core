/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_DATABASE_DATABASE_H_
#define BRAVELEDGER_DATABASE_DATABASE_H_

#include <stdint.h>

#include <map>
#include <memory>
#include <string>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_database {

class Database {
 public:
  explicit Database(bat_ledger::LedgerImpl* ledger);

  ~Database();

  void Initialize();

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_database
#endif  // BRAVELEDGER_DATABASE_DATABASE_H_
