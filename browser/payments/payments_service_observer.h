// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_OBSERVER_H_
#define BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_OBSERVER_H_

#include "brave/browser/payments/wallet_properties.h"
#include "brave/browser/payments/promotion.h"

namespace payments {

class PaymentsService;

class PaymentsServiceObserver {
 public:
  virtual ~PaymentsServiceObserver() {}

  virtual void OnWalletCreated(PaymentsService* payment_service,
                               int error_code) {};
  virtual void OnWalletProperties(PaymentsService* payment_service,
                                  payments::WalletProperties properties) {};
  virtual void OnPromotion(PaymentsService* payment_service,
                           payments::Promotion properties) {};
  virtual void OnPromotionCaptcha(PaymentsService* payment_service,
                                  std::string image) {};
  virtual void OnRecoverWallet(PaymentsService* payment_service,
                               bool error,
                               double balance) {};
};

}  // namespace payments

#endif  // BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_OBSERVER_H_
