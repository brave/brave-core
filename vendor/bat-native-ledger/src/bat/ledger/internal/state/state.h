/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_BAT_STATE_STATE_H_
#define BRAVELEDGER_BAT_STATE_STATE_H_

#include <memory>
#include <string>
#include <vector>

namespace bat_ledger {
class LedgerImpl;
}

namespace braveledger_state {

class StateMigration;

class State {
 public:
  explicit State(bat_ledger::LedgerImpl* ledger);
  ~State();

  void Initialize(ledger::ResultCallback callback);

  void SetVersion(const int version);

  int GetVersion();

  void SetPublisherMinVisitTime(const int duration);

  int GetPublisherMinVisitTime();

  void SetPublisherMinVisits(const int visits);

  int GetPublisherMinVisits();

  void SetPublisherAllowNonVerified(const bool allow);

  bool GetPublisherAllowNonVerified();

  void SetPublisherAllowVideos(const bool allow);

  bool GetPublisherAllowVideos();

  void SetScoreValues(double a, double b);

  void GetScoreValues(double* a, double* b);

  void SetRewardsMainEnabled(bool enabled);

  bool GetRewardsMainEnabled();

  void SetAutoContributeEnabled(bool enabled);

  bool GetAutoContributeEnabled();

  void SetAutoContributionAmount(const double amount);

  double GetAutoContributionAmount();

  uint64_t GetReconcileStamp();

  void SetReconcileStamp(const int reconcile_interval);

  void ResetReconcileStamp();

  uint64_t GetCreationStamp();

  void SetCreationStamp(const uint64_t stamp);

  std::vector<uint8_t> GetRecoverySeed();

  void SetRecoverySeed(const std::vector<uint8_t>& seed);

  std::string GetPaymentId();

  void SetPaymentId(const std::string& id);

  bool GetInlineTippingPlatformEnabled(
      const ledger::InlineTipsPlatforms platform);

  void SetInlineTippingPlatformEnabled(
      const ledger::InlineTipsPlatforms platform,
      const bool enabled);

  void SetRewardsParameters(const ledger::RewardsParameters& parameters);

  ledger::RewardsParametersPtr GetRewardsParameters();

  double GetRate();

  double GetAutoContributeChoice();

  std::vector<double> GetAutoContributeChoices();

  std::vector<double> GetTipChoices();

  std::vector<double> GetMonthlyTipChoices();

  void SetFetchOldBalanceEnabled(bool enabled);

  bool GetFetchOldBalanceEnabled();

  void SetEmptyBalanceChecked(const bool checked);

  bool GetEmptyBalanceChecked();

  void SetServerPublisherListStamp(const uint64_t stamp);

  uint64_t GetServerPublisherListStamp();

  void SetPromotionCorruptedMigrated(const bool migrated);

  bool GetPromotionCorruptedMigrated();

  void SetPromotionLastFetchStamp(const uint64_t stamp);

  uint64_t GetPromotionLastFetchStamp();

  void SetAnonTransferChecked(const bool checked);

  bool GetAnonTransferChecked();

 private:
  bat_ledger::LedgerImpl* ledger_;  // NOT OWNED
  std::unique_ptr<StateMigration> migration_;
};

}  // namespace braveledger_state

#endif  // BRAVELEDGER_BAT_STATE_STATE_H_
