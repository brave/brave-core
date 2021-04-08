/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/payments/content/bat_payment_app_factory.h"

#include <utility>

#include "brave/components/brave_rewards/common/constants.h"
#include "brave/components/payments/content/bat_payment_app.h"
#include "components/payments/content/content_payment_request_delegate.h"
#include "components/payments/content/payment_request.h"
#include "components/payments/content/payment_request_spec.h"

namespace payments {

BatPaymentAppFactory::BatPaymentAppFactory()
    : PaymentAppFactory(PaymentApp::Type::INTERNAL) {}

BatPaymentAppFactory::~BatPaymentAppFactory() = default;

// static
bool BatPaymentAppFactory::IsBatSupportedMethod(
    base::WeakPtr<PaymentRequestSpec> spec) {
  for (const auto& data : spec->method_data()) {
    if (data->supported_method == brave_rewards::kBatPaymentMethod) {
      return true;
    }
  }
  return false;
}

// static
bool BatPaymentAppFactory::IsBatSupportedMethod(
    base::WeakPtr<PaymentRequest> request) {
  return BatPaymentAppFactory::IsBatSupportedMethod(request->spec());
}

void BatPaymentAppFactory::Create(base::WeakPtr<Delegate> delegate) {
  DCHECK(delegate);
  auto spec = delegate->GetSpec();
  if (!spec)
    return;

  if (BatPaymentAppFactory::IsBatSupportedMethod(spec)) {
    auto app = std::make_unique<BatPaymentApp>(spec);
    if (app) {
      delegate->OnPaymentAppCreated(std::move(app));
    }
  }
  delegate->OnDoneCreatingPaymentApps();
}

}  // namespace payments
