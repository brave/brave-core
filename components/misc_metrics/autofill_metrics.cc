/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/misc_metrics/autofill_metrics.h"

#include "base/metrics/histogram_macros.h"
#include "components/autofill/core/browser/payments_data_manager.h"
#include "components/autofill/core/browser/personal_data_manager.h"

namespace misc_metrics {

AutofillMetrics::AutofillMetrics(
    autofill::PersonalDataManager* personal_data_manager)
    : personal_data_manager_(personal_data_manager), observation_(this) {
  CHECK(personal_data_manager);
  observation_.Observe(personal_data_manager);
  ReportMetric();
}

AutofillMetrics::~AutofillMetrics() = default;

void AutofillMetrics::OnPersonalDataChanged() {
  ReportMetric();
}

void AutofillMetrics::ReportMetric() {
  if (!personal_data_manager_->IsDataLoaded()) {
    return;
  }
  auto cards = personal_data_manager_->payments_data_manager().GetCreditCards();
  UMA_HISTOGRAM_BOOLEAN(kPaymentMethodPresentHistogramName, !cards.empty());
}

}  // namespace misc_metrics
