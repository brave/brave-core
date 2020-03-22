/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_ATTESTATION_CHANNEL_ONE_H_
#define BRAVELEDGER_ATTESTATION_CHANNEL_ONE_H_

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_attestation_channel {

static const uint64_t timers[] = {
    1, // 2s
    1 * 60, // 60s
    1 * 60 * 60,  // 1h
    2 * 60 * 60,  // 2h
    12 * 60 * 60,  // 12h
    24 * 60 * 60,  // 24h
    48 * 60 * 60};  // 48h

//using ResponseCallback = std::function<void(const ledger::Result, const std::string&)>;

class PrivateChannelOne {
  public:
    explicit PrivateChannelOne(bat_ledger::LedgerImpl* ledger);
    
    ~PrivateChannelOne();

    void Initialize();

    void SetTimer(uint32_t* timer_id, uint64_t start_timer_in = 0);

    void OnTimer(uint32_t timer_id);

  private:
  void StartProtocol();

  void SecondRoundProtocol();

  void OnFirstRoundResponse(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

  void OnSecondRoundResponse(
    int response_status_code,
    const std::string& response,
    const std::map<std::string, std::string>& headers);

    bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
    uint32_t attestation_timer_id_;
};

} // braveledger_attestation_channel

#endif  // BRAVELEDGER_ATTESTATION_CHANNEL_ONE_H_
