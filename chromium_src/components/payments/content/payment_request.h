/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_PAYMENTS_CONTENT_PAYMENT_REQUEST_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_PAYMENTS_CONTENT_PAYMENT_REQUEST_H_

#include "build/buildflag.h"
#include "brave/components/payments/content/buildflags/buildflags.h"
#include "brave/vendor/bat-native-ledger/include/bat/ledger/mojom_structs.h"
#include "components/payments/content/payment_request_state.h"
#include "third_party/blink/public/mojom/payments/payment_request.mojom.h"

#if BUILDFLAG(ENABLE_PAY_WITH_BAT)
#define BRAVE_PAYMENT_METHOD_UTIL_FUNCTIONS                                  \
  void OnError(payments::mojom::PaymentErrorReason reason,                   \
               std::string err);
#define Init                                                                 \
  Init_ChromiumImpl(mojo::PendingRemote<mojom::PaymentRequestClient> client, \
                    std::vector<mojom::PaymentMethodDataPtr> method_data,    \
                    mojom::PaymentDetailsPtr details,                        \
                    mojom::PaymentOptionsPtr options);                       \
  void Init
#else
#define BRAVE_PAYMENT_METHOD_UTIL_FUNCTIONS
#endif

#include "../../../../../components/payments/content/payment_request.h"
#if BUILDFLAG(ENABLE_PAY_WITH_BAT)
#undef Init
#endif

#undef BRAVE_PAYMENT_METHOD_UTIL_FUNCTIONS

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_PAYMENTS_CONTENT_PAYMENT_REQUEST_H_
