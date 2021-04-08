/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/payments/content/payment_request.h"

#include "brave/components/brave_rewards/common/constants.h"
#include "brave/components/payments/content/buildflags/buildflags.h"
#include "third_party/blink/public/mojom/payments/payment_request.mojom.h"

using payments::mojom::PaymentErrorReason;

#if BUILDFLAG(ENABLE_PAY_WITH_BAT)
namespace payments {

void PaymentRequest::OnError(PaymentErrorReason reason, std::string err) {
  client_->OnError(reason, err);
}

void PaymentRequest::Init(
    mojo::PendingRemote<mojom::PaymentRequestClient> client,
    std::vector<mojom::PaymentMethodDataPtr> method_data,
    mojom::PaymentDetailsPtr details,
    mojom::PaymentOptionsPtr options) {
  std::vector<mojom::PaymentMethodDataPtr>::iterator it = std::find_if(
      method_data.begin(), method_data.end(), [](const auto& method) {
        return method->supported_method == brave_rewards::kBatPaymentMethod;
      });

  // Only BAT payment method needs additional checks
  if (it == method_data.end()) {
    Init_ChromiumImpl(std::move(client), std::move(method_data),
                      std::move(details), std::move(options));
    return;
  }

  // For BAT payment method, every item should have an associated SKU token
  if (details->display_items.has_value()) {
    for (const mojom::PaymentItemPtr& display_item : *details->display_items) {
      if (!(display_item->sku.has_value())) {
        log_.Error(brave_rewards::errors::kInvalidData);
        TerminateConnection();
        return;
      }
    }
  } else {
    log_.Error(brave_rewards::errors::kInvalidData);
    TerminateConnection();
    return;
  }

  Init_ChromiumImpl(std::move(client), std::move(method_data),
                    std::move(details), std::move(options));
}

}  // namespace payments

#define Init Init_ChromiumImpl
#endif
#include "../../../../../components/payments/content/payment_request.cc"

#if BUILDFLAG(ENABLE_PAY_WITH_BAT)
#undef Init
#endif
