/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/contribution/uphold.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace braveledger_contribution {

Uphold::Uphold(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
}

Uphold::~Uphold() {
}

void Uphold::Start(const std::string &viewing_id, const std::string& token) {
  const auto reconcile = ledger_->GetReconcileById(viewing_id);

  // TODO check if token is valid


}

}  // namespace braveledger_contribution
