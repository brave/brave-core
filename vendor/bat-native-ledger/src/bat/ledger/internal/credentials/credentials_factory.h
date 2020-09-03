/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_CREDENTIALS_FACTORY_H_
#define BRAVELEDGER_CREDENTIALS_FACTORY_H_

#include <memory>

#include "bat/ledger/internal/credentials/credentials.h"
#include "bat/ledger/ledger.h"

namespace ledger {
class LedgerImpl;

namespace credential {

class CredentialsFactory {
 public:
  static std::unique_ptr<Credentials> Create(
      LedgerImpl* ledger,
      const type::CredsBatchType trigger_type);
};

}  // namespace credential
}  // namespace ledger

#endif  // BRAVELEDGER_CREDENTIALS_FACTORY_H_
