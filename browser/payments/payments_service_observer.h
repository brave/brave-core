// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_OBSERVER_H_
#define BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_OBSERVER_H_

#include "bat/ledger/ledger.h"
#include "bat/ledger/wallet_info.h"

namespace payments {

class PaymentsService;

class PaymentsServiceObserver {
 public:
  virtual ~PaymentsServiceObserver() {}

  virtual void OnWalletCreated(PaymentsService* payment_service,
                               int error_code) {};
  virtual void OnWalletProperties(PaymentsService* payment_service,
                               ledger::WalletInfo properties) {};
};

}  // namespace payments

#endif  // BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_OBSERVER_H_
