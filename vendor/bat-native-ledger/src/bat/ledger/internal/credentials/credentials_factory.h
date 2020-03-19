/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CREDENTIALS_FACTORY_H_
#define BRAVELEDGER_CREDENTIALS_FACTORY_H_

#include <memory>

#include "bat/ledger/internal/credentials/credentials.h"
#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_credentials {

class CredentialsFactory {
 public:
  static std::unique_ptr<Credentials> Create(
      bat_ledger::LedgerImpl* ledger,
      const ledger::CredsBatchType trigger_type);
};

}  // namespace braveledger_credentials

#endif  // BRAVELEDGER_CREDENTIALS_FACTORY_H_
