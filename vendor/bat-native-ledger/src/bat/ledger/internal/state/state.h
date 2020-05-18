/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_STATE_STATE_H_
#define BRAVELEDGER_BAT_STATE_STATE_H_

#include <memory>

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_state {

class StateMigration;

class State {
 public:
  explicit State(bat_ledger::LedgerImpl* ledger);
  ~State();

  void Initialize(ledger::ResultCallback callback);

 private:
  std::unique_ptr<StateMigration> migration_;
};

}  // namespace braveledger_state

#endif  // BRAVELEDGER_BAT_STATE_STATE_H_
