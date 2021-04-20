/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/payments/content/payment_app_service.h"

#include <memory>

#include "brave/components/payments/content/bat_payment_app_factory.h"
#include "brave/components/payments/content/buildflags/buildflags.h"

#define PaymentAppService PaymentAppService_ChromiumImpl
#include "../../../../../components/payments/content/payment_app_service.cc"
#undef PaymentAppService

namespace payments {

PaymentAppService::PaymentAppService(content::BrowserContext* context)
    : PaymentAppService_ChromiumImpl(context) {
#if BUILDFLAG(ENABLE_PAY_WITH_BAT)
  factories_.emplace_back(std::make_unique<BatPaymentAppFactory>());
#endif
}

}  // namespace payments
