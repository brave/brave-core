/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_PUBLISHER_STATE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_PUBLISHER_STATE_H_

#include <map>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/ledger_callbacks.h"
#include "brave/components/brave_rewards/core/legacy/publisher_settings_properties.h"

namespace brave_rewards::internal::publisher {

class LegacyPublisherState {
 public:
  uint64_t GetPublisherMinVisitTime() const;  // In milliseconds

  unsigned int GetPublisherMinVisits() const;

  bool GetPublisherAllowNonVerified() const;

  void Load(LegacyResultCallback callback);

  std::vector<std::string> GetAlreadyProcessedPublishers() const;

  void GetAllBalanceReports(std::vector<mojom::BalanceReportInfoPtr>* reports);

 private:
  void OnLoad(LegacyResultCallback callback,
              mojom::Result result,
              const std::string& data);

  PublisherSettingsProperties state_;
};

}  // namespace brave_rewards::internal::publisher

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_PUBLISHER_STATE_H_
