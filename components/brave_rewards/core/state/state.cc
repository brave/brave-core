/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/state/state.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/base64.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/constants.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/parameters/rewards_parameters_provider.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/state/state_migration.h"

namespace brave_rewards::internal::state {

State::State(RewardsEngineImpl& engine) : engine_(engine), migration_(engine) {}

State::~State() = default;

void State::Initialize(ResultCallback callback) {
  migration_.Start(std::move(callback));
}

void State::SetVersion(const int version) {
  engine_->database()->SaveEventLog(kVersion, std::to_string(version));
  engine_->SetState(kVersion, version);
}

int State::GetVersion() {
  return engine_->GetState<int>(kVersion);
}

void State::SetPublisherMinVisitTime(const int duration) {
  engine_->database()->SaveEventLog(kMinVisitTime, std::to_string(duration));
  engine_->SetState(kMinVisitTime, duration);
  engine_->publisher()->CalcScoreConsts(duration);
  engine_->publisher()->SynopsisNormalizer();
}

int State::GetPublisherMinVisitTime() {
  return engine_->GetState<int>(kMinVisitTime);
}

void State::SetPublisherMinVisits(const int visits) {
  engine_->database()->SaveEventLog(kMinVisits, std::to_string(visits));
  engine_->SetState(kMinVisits, visits);
  engine_->publisher()->SynopsisNormalizer();
}

int State::GetPublisherMinVisits() {
  return engine_->GetState<int>(kMinVisits);
}

void State::SetScoreValues(double a, double b) {
  engine_->database()->SaveEventLog(kScoreA, std::to_string(a));
  engine_->database()->SaveEventLog(kScoreB, std::to_string(b));
  engine_->SetState(kScoreA, a);
  engine_->SetState(kScoreB, b);
}

void State::GetScoreValues(double* a, double* b) {
  DCHECK(a && b);
  *a = engine_->GetState<double>(kScoreA);
  *b = engine_->GetState<double>(kScoreB);
}

void State::SetAutoContributeEnabled(bool enabled) {
  // If AC is not supported, then always set the pref to false.
  if (!engine_->IsAutoContributeSupportedForClient()) {
    enabled = false;
  }

  engine_->database()->SaveEventLog(kAutoContributeEnabled,
                                    std::to_string(enabled));
  engine_->SetState(kAutoContributeEnabled, enabled);

  if (enabled) {
    engine_->publisher()->CalcScoreConsts(GetPublisherMinVisitTime());
  }
}

bool State::GetAutoContributeEnabled() {
  // If AC is not supported, then always report AC as disabled.
  if (!engine_->IsAutoContributeSupportedForClient()) {
    return false;
  }

  return engine_->GetState<bool>(kAutoContributeEnabled);
}

void State::SetAutoContributionAmount(const double amount) {
  engine_->database()->SaveEventLog(kAutoContributeAmount,
                                    std::to_string(amount));
  engine_->SetState(kAutoContributeAmount, amount);
}

double State::GetAutoContributionAmount() {
  double amount = engine_->GetState<double>(kAutoContributeAmount);
  if (amount == 0.0) {
    auto params = engine_->Get<RewardsParametersProvider>().LoadParameters();
    if (params) {
      amount = params->auto_contribute_choice;
    }
  }

  return amount;
}

uint64_t State::GetReconcileStamp() {
  auto stamp = engine_->GetState<uint64_t>(kNextReconcileStamp);
  if (stamp == 0) {
    ResetReconcileStamp();
    stamp = engine_->GetState<uint64_t>(kNextReconcileStamp);
  }

  return stamp;
}

void State::SetReconcileStamp(const int reconcile_interval) {
  uint64_t reconcile_stamp = util::GetCurrentTimeStamp();
  if (reconcile_interval > 0) {
    reconcile_stamp += reconcile_interval * 60;
  } else {
    reconcile_stamp += constant::kReconcileInterval;
  }

  engine_->database()->SaveEventLog(kNextReconcileStamp,
                                    std::to_string(reconcile_stamp));
  engine_->SetState(kNextReconcileStamp, reconcile_stamp);
  engine_->client()->ReconcileStampReset();
}
void State::ResetReconcileStamp() {
  SetReconcileStamp(engine_->options().reconcile_interval);
}

uint64_t State::GetCreationStamp() {
  return engine_->GetState<uint64_t>(kCreationStamp);
}

void State::SetCreationStamp(const uint64_t stamp) {
  engine_->database()->SaveEventLog(kCreationStamp, std::to_string(stamp));
  engine_->SetState(kCreationStamp, stamp);
}

void State::SetEmptyBalanceChecked(const bool checked) {
  engine_->database()->SaveEventLog(kEmptyBalanceChecked,
                                    std::to_string(checked));
  engine_->SetState(kEmptyBalanceChecked, checked);
}

bool State::GetEmptyBalanceChecked() {
  return engine_->GetState<bool>(kEmptyBalanceChecked);
}

void State::SetServerPublisherListStamp(const uint64_t stamp) {
  engine_->SetState(kServerPublisherListStamp, stamp);
}

uint64_t State::GetServerPublisherListStamp() {
  return engine_->GetState<uint64_t>(kServerPublisherListStamp);
}

void State::SetPromotionCorruptedMigrated(const bool migrated) {
  engine_->database()->SaveEventLog(kPromotionCorruptedMigrated,
                                    std::to_string(migrated));
  engine_->SetState(kPromotionCorruptedMigrated, migrated);
}

bool State::GetPromotionCorruptedMigrated() {
  return engine_->GetState<bool>(kPromotionCorruptedMigrated);
}

void State::SetPromotionLastFetchStamp(const uint64_t stamp) {
  engine_->SetState(kPromotionLastFetchStamp, stamp);
}

uint64_t State::GetPromotionLastFetchStamp() {
  return engine_->GetState<uint64_t>(kPromotionLastFetchStamp);
}

std::optional<std::string> State::GetEncryptedString(const std::string& key) {
  std::string value = engine_->GetState<std::string>(key);

  // If the state value is empty, then we consider this a successful read of a
  // default empty string.
  if (value.empty()) {
    return "";
  }

  if (!base::Base64Decode(value, &value)) {
    engine_->LogError(FROM_HERE) << "Base64 decoding failed for " << key;
    return {};
  }

  auto decrypted = engine_->DecryptString(value);
  if (!decrypted) {
    engine_->LogError(FROM_HERE) << "Decryption failed for " << key;
    return {};
  }

  return *decrypted;
}

bool State::SetEncryptedString(const std::string& key,
                               const std::string& value) {
  auto encrypted = engine_->EncryptString(value);
  if (!encrypted) {
    engine_->LogError(FROM_HERE) << "Encryption failed for " << key;
    return false;
  }

  std::string base64_string;
  base::Base64Encode(*encrypted, &base64_string);

  engine_->SetState(key, std::move(base64_string));
  return true;
}

}  // namespace brave_rewards::internal::state
