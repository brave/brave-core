/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PAYMENTS_CONTENT_PAYMENT_APP_SERVICE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PAYMENTS_CONTENT_PAYMENT_APP_SERVICE_H_

#include <vector>

#include "components/payments/content/payment_app_factory.h"

#define PaymentAppService PaymentAppService_ChromiumImpl
#define factories_ \
  unused_;         \
                   \
 protected:        \
  std::vector<std::unique_ptr<PaymentAppFactory>> factories_
#include "../../../../../components/payments/content/payment_app_service.h"
#undef factories_
#undef PaymentAppService

namespace payments {

class PaymentAppService : public PaymentAppService_ChromiumImpl {
 public:
  explicit PaymentAppService(content::BrowserContext* context);
};

}  // namespace payments

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PAYMENTS_CONTENT_PAYMENT_APP_SERVICE_H_
