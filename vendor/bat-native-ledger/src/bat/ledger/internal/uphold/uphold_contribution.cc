/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/uphold/uphold_contribution.h"
#include "bat/ledger/internal/ledger_impl.h"

namespace braveledger_uphold {

UpholdContribution::UpholdContribution(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
}

UpholdContribution::~UpholdContribution() {
}

void UpholdContribution::Start(const std::string &viewing_id,
                               ledger::ExternalWallet wallet) {
  const auto reconcile = ledger_->GetReconcileById(viewing_id);
  // TODO finish me
}

}  // namespace braveledger_uphold
