/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COVARIATES_COVARIATE_MANAGER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COVARIATES_COVARIATE_MANAGER_H_

#include <memory>
#include <vector>

#include "base/containers/flat_map.h"
#include "bat/ads/internal/covariates/covariate_log_entry_interface.h"
#include "bat/ads/public/interfaces/ads.mojom-shared.h"
#include "brave/components/brave_federated/public/interfaces/brave_federated.mojom-forward.h"

namespace base {
class Time;
}  // namespace base

namespace ads {

// |CovariateManager| collects training data for federated services such as
// learning, tuning and evaluation. A row in the training data set is called
// "instance ". A column is called a "feature". To differentiate between
// Chromium/Griffin features and federated services features, we call them
// covariates instead. Covariate values can be of different data types as
// defined in |mojom::CovariateInfo|. All covariates are only session based at
// the moment, i.e no measurements are persisted across sessions.
class CovariateManager final {
 public:
  CovariateManager();

  CovariateManager(const CovariateManager& other) = delete;
  CovariateManager& operator=(const CovariateManager& other) = delete;

  CovariateManager(CovariateManager&& other) noexcept = delete;
  CovariateManager& operator=(CovariateManager&& other) noexcept = delete;

  ~CovariateManager();

  static CovariateManager* GetInstance();

  static bool HasInstance();

  void SetLogEntry(std::unique_ptr<CovariateLogEntryInterface> entry);
  std::vector<brave_federated::mojom::CovariateInfoPtr> GetTrainingInstance()
      const;

  void SetNotificationAdServedAt(base::Time time);
  void SetNotificationAdEvent(mojom::NotificationAdEventType event_type);
  void LogTrainingInstance() const;

 private:
  base::flat_map<brave_federated::mojom::CovariateType,
                 std::unique_ptr<CovariateLogEntryInterface>>
      covariate_log_entries_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_COVARIATES_COVARIATE_MANAGER_H_
