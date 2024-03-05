/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_H_

#include <optional>
#include <string>
#include <vector>

#include "base/containers/flat_map.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_rewards/core/state/state_migration.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace state {

class State {
 public:
  explicit State(RewardsEngineImpl& engine);
  ~State();

  void Initialize(ResultCallback callback);

  void SetVersion(const int version);

  int GetVersion();

  void SetPublisherMinVisitTime(const int duration);

  int GetPublisherMinVisitTime();

  void SetPublisherMinVisits(const int visits);

  int GetPublisherMinVisits();

  void SetScoreValues(double a, double b);

  void GetScoreValues(double* a, double* b);

  void SetAutoContributeEnabled(bool enabled);

  bool GetAutoContributeEnabled();

  void SetAutoContributionAmount(const double amount);

  double GetAutoContributionAmount();

  uint64_t GetReconcileStamp();

  void SetReconcileStamp(const int reconcile_interval);

  void ResetReconcileStamp();

  uint64_t GetCreationStamp();

  void SetCreationStamp(const uint64_t stamp);

  void SetRewardsParameters(const mojom::RewardsParameters& parameters);

  mojom::RewardsParametersPtr GetRewardsParameters();

  double GetRate();

  double GetAutoContributeChoice();

  std::vector<double> GetAutoContributeChoices();

  std::vector<double> GetTipChoices();

  std::vector<double> GetMonthlyTipChoices();

  base::flat_map<std::string, std::string> GetPayoutStatus();

  base::flat_map<std::string, mojom::RegionsPtr> GetWalletProviderRegions();

  base::Time GetVBatDeadline();

  bool GetVBatExpired();

  void SetServerPublisherListStamp(const uint64_t stamp);

  uint64_t GetServerPublisherListStamp();

  std::optional<std::string> GetEncryptedString(const std::string& key);

  bool SetEncryptedString(const std::string& key, const std::string& value);

 private:
  const raw_ref<RewardsEngineImpl> engine_;
  StateMigration migration_;
};

}  // namespace state
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_STATE_STATE_H_
