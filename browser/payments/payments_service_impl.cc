/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#include "brave/browser/payments/payments_service_impl.h"

#include "bat/ledger/ledger.h"

namespace payments {

  PaymentsServiceImpl::PaymentsServiceImpl() :
      ledger_(new braveledger_ledger::Ledger()) {
  }

  PaymentsServiceImpl::~PaymentsServiceImpl() {
  }

  void PaymentsServiceImpl::CreateWallet() {
    ledger_->createWallet();
  }

  void PaymentsServiceImpl::Shutdown() {
    ledger_.reset();
    PaymentsService::Shutdown();
  }
}  // namespace history
