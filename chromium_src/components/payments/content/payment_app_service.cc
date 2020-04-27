/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/payments/content/bat_payment_app.h"
#include "components/payments/content/payment_request_state.h"

namespace {

using payments::BatPaymentApp;
using payments::kBatPaymentAppType;
using payments::kBatPaymentMethod;
using payments::PaymentAppFactory;

class BatPaymentAppFactory : public PaymentAppFactory {
 public:
  BatPaymentAppFactory() : PaymentAppFactory(kBatPaymentAppType) {}
  ~BatPaymentAppFactory() override = default;

  void Create(base::WeakPtr<Delegate> delegate) override {
    auto& methods = delegate->GetSpec()->payment_method_identifiers_set();
    if (methods.count(kBatPaymentMethod) > 0) {
      delegate->OnPaymentAppCreated(std::make_unique<BatPaymentApp>());
    }
    delegate->OnDoneCreatingPaymentApps();
  }
};

}  // namespace

#define BRAVE_ADD_BAT_PAYMENT_APP_FACTORY \
    factories_.emplace_back(std::make_unique<BatPaymentAppFactory>());

#include "../../../../../components/payments/content/payment_app_service.cc"

#undef BRAVE_ADD_BAT_PAYMENT_APP_FACTORY
