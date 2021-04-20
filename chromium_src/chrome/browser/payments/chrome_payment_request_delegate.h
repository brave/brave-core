/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PAYMENTS_CHROME_PAYMENT_REQUEST_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PAYMENTS_CHROME_PAYMENT_REQUEST_DELEGATE_H_

#define ChromePaymentRequestDelegate ChromePaymentRequestDelegate_ChromiumImpl
#include "../../../../../chrome/browser/payments/chrome_payment_request_delegate.h"  // NOLINT
#undef ChromePaymentRequestDelegate

namespace content {
class RenderFrameHost;
}

namespace payments {

class PaymentRequest;

class ChromePaymentRequestDelegate : public ChromePaymentRequestDelegate_ChromiumImpl {
 public:
  explicit ChromePaymentRequestDelegate(
      content::RenderFrameHost* render_frame_host);
  ~ChromePaymentRequestDelegate() override;

  void ShowDialog(base::WeakPtr<PaymentRequest> request) override;
};

}  // namespace payments

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_PAYMENTS_CHROME_PAYMENT_REQUEST_DELEGATE_H_
