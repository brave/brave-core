/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ATTESTATION_CHANNEL_H_
#define BRAVELEDGER_ATTESTATION_CHANNEL_H_

#include <map>
#include <string>

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_attestation_channel {

using StartCallback =
    std::function<void(const ledger::Result, const std::string&)>;

class PrivateAttestationChannel {
 public:
  explicit PrivateAttestationChannel(bat_ledger::LedgerImpl* ledger);
  virtual ~PrivateAttestationChannel();

  virtual void SetTimer(uint32_t* timer_id, uint64_t start_timer_in) = 0;

  virtual void OnTimer(uint32_t timer_id) = 0;


 protected:
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  uint32_t attestation_timer_id_;

};

}  // namespace braveledger_attestation_channel
#endif  // BRAVELEDGER_ATTESTATION_CHANNEL_H_
