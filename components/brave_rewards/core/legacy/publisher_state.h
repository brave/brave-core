/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_PUBLISHER_STATE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_PUBLISHER_STATE_H_

#include <map>
#include <memory>
#include <string>
#include <vector>

#include "brave/components/brave_rewards/core/ledger.h"
#include "brave/components/brave_rewards/core/legacy/publisher_settings_properties.h"

namespace ledger {
class LedgerImpl;
}

namespace ledger {
namespace publisher {

class LegacyPublisherState {
 public:
  explicit LegacyPublisherState(ledger::LedgerImpl* ledger);

  ~LegacyPublisherState();

  uint64_t GetPublisherMinVisitTime() const;  // In milliseconds

  unsigned int GetPublisherMinVisits() const;

  bool GetPublisherAllowNonVerified() const;

  void Load(ledger::LegacyResultCallback callback);

  std::vector<std::string> GetAlreadyProcessedPublishers() const;

  void GetAllBalanceReports(
      std::vector<ledger::mojom::BalanceReportInfoPtr>* reports);

 private:
  void OnLoad(ledger::LegacyResultCallback callback,
              ledger::mojom::Result result,
              const std::string& data);

  ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<ledger::PublisherSettingsProperties> state_;
};

}  // namespace publisher
}  // namespace ledger

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_LEGACY_PUBLISHER_STATE_H_
