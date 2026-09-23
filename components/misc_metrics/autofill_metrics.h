/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_MISC_METRICS_AUTOFILL_METRICS_H_
#define BRAVE_COMPONENTS_MISC_METRICS_AUTOFILL_METRICS_H_

#include "base/memory/raw_ptr.h"
#include "base/scoped_observation.h"
#include "components/autofill/core/browser/data_manager/personal_data_manager_observer.h"

namespace autofill {
class PersonalDataManager;
}

namespace misc_metrics {

inline constexpr char kPaymentMethodPresentHistogramName[] =
    "Brave.Autofill.PaymentMethodPresent";

// Reports broad metrics regarding autofill settings.
// Currently this reports a simple boolean metric regarding
// whether a payment method is present.
class AutofillMetrics : public autofill::PersonalDataManagerObserver {
 public:
  explicit AutofillMetrics(
      autofill::PersonalDataManager* personal_data_manager);
  ~AutofillMetrics() override;

  AutofillMetrics(const AutofillMetrics&) = delete;
  AutofillMetrics& operator=(const AutofillMetrics&) = delete;

 private:
  // autofill::PersonalDataManagerObserver:
  void OnPersonalDataChanged() override;

  void ReportMetric();

  raw_ptr<autofill::PersonalDataManager> personal_data_manager_;
  base::ScopedObservation<autofill::PersonalDataManager, AutofillMetrics>
      observation_;
};

}  // namespace misc_metrics

#endif  // BRAVE_COMPONENTS_MISC_METRICS_AUTOFILL_METRICS_H_
