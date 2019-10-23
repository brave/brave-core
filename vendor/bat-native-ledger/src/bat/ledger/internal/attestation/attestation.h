/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ATTESTATION_ATTESTATION_H_
#define BRAVELEDGER_ATTESTATION_ATTESTATION_H_

#include <map>
#include <string>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_attestation {

using StartCallback =
    std::function<void(const ledger::Result, const std::string&)>;

using ConfirmCallback =
    std::function<void(const ledger::Result)>;

class Attestation {
 public:
  explicit Attestation(bat_ledger::LedgerImpl* ledger);
  virtual ~Attestation();

  virtual void Start(const std::string& payload, StartCallback callback) = 0;

  virtual void Confirm(const std::string& result, ConfirmCallback callback) = 0;

 protected:
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
};

}  // namespace braveledger_attestation
#endif  // BRAVELEDGER_ATTESTATION_ATTESTATION_H_
