/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/payments/chrome_payment_request_delegate.h"

#include "brave/browser/ui/brave_rewards/checkout_dialog.h"
#include "brave/components/payments/content/bat_payment_app_factory.h"

#define ChromePaymentRequestDelegate ChromePaymentRequestDelegate_ChromiumImpl
#include "../../../../../chrome/browser/payments/chrome_payment_request_delegate.cc"  // NOLINT
#undef ChromePaymentRequestDelegate

namespace payments {

ChromePaymentRequestDelegate::ChromePaymentRequestDelegate(
    content::RenderFrameHost* render_frame_host)
    : ChromePaymentRequestDelegate_ChromiumImpl(render_frame_host) {}

ChromePaymentRequestDelegate::~ChromePaymentRequestDelegate() = default;

void ChromePaymentRequestDelegate::ShowDialog(
    base::WeakPtr<PaymentRequest> request) {
  if (BatPaymentAppFactory::IsBatSupportedMethod(request)) {
  	brave_rewards::ShowCheckoutDialog(request);
    return;
  }
  ChromePaymentRequestDelegate_ChromiumImpl::ShowDialog(request);
}

}  // namespace payments
