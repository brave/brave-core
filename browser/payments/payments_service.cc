/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/browser/payments/payments_service.h"

#include "bat/ledger/ledger.h"

namespace payments {

  PaymentsService::PaymentsService() :
      ledger_(new braveledger_ledger::Ledger()) {
  }

  PaymentsService::~PaymentsService() {
  }

  void PaymentsService::CreateWallet() {
    ledger_->createWallet();
  }

  void PaymentsService::Shutdown() {
    ledger_.reset();
  }
}  // namespace history
