/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/autofill/core/browser/payments/credit_card_access_manager.h"

#include "components/autofill/core/browser/metrics/payments/card_unmask_flow_metrics.h"

namespace autofill::autofill_metrics {
void BraveLogServerCardUnmaskAttempt(
    payments::PaymentsAutofillClient::PaymentsRpcCardType card_type);
}  // namespace autofill::autofill_metrics

#define LogServerCardUnmaskAttempt BraveLogServerCardUnmaskAttempt
#include "src/components/autofill/core/browser/payments/credit_card_access_manager.cc"
#undef LogServerCardUnmaskAttempt

namespace autofill::autofill_metrics {

void BraveLogServerCardUnmaskAttempt(
    payments::PaymentsAutofillClient::PaymentsRpcCardType card_type) {
  // Do not log kMaskedServerCard or kFullServerCard. These used to be excluded
  // by kAutofillEnableRemadeDownstreamMetrics feature flag that was removed in
  // Chromium 128.
  if (card_type ==
      payments::PaymentsAutofillClient::PaymentsRpcCardType::kVirtualCard) {
    LogServerCardUnmaskAttempt(card_type);
  }
}

}  // namespace autofill::autofill_metrics
