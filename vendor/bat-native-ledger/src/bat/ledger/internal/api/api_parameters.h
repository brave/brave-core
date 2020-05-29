/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_API_API_PARAMETERS_H_
#define BRAVELEDGER_API_API_PARAMETERS_H_

#include "bat/ledger/ledger.h"

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_api {

class APIParameters {
 public:
  explicit APIParameters(bat_ledger::LedgerImpl* ledger);
  ~APIParameters();

  void Initialize();

  void OnTimer(const uint32_t timer_id);

  void Fetch();

 private:
  void OnFetch(const ledger::UrlResponse& response);

  void SetRefreshTimer(const int delay = 0);

  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  uint32_t refresh_timer_id_;
};

}  // namespace braveledger_api

#endif  // BRAVELEDGER_API_API_PARAMETERS_H_
