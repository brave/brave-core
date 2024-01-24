/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/state/state.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/base64.h"
#include "base/containers/contains.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "brave/components/brave_rewards/core/common/time_util.h"
#include "brave/components/brave_rewards/core/constants.h"
#include "brave/components/brave_rewards/core/database/database.h"
#include "brave/components/brave_rewards/core/endpoints/brave/get_parameters_utils.h"
#include "brave/components/brave_rewards/core/publisher/publisher.h"
#include "brave/components/brave_rewards/core/rewards_engine_impl.h"
#include "brave/components/brave_rewards/core/state/state_keys.h"
#include "brave/components/brave_rewards/core/state/state_migration.h"

namespace brave_rewards::internal {

namespace {

std::string VectorDoubleToString(const std::vector<double>& items) {
  base::Value::List list;
  for (const auto& item : items) {
    list.Append(item);
  }

  std::string items_string;
  base::JSONWriter::Write(list, &items_string);

  return items_string;
}

std::vector<double> StringToVectorDouble(const std::string& items_string) {
  std::optional<base::Value> list = base::JSONReader::Read(items_string);
  if (!list || !list->is_list()) {
    return {};
  }

  auto& list_value = list->GetList();
  std::vector<double> items;
  for (auto& item : list_value) {
    if (!item.is_double()) {
      continue;
    }
    items.push_back(item.GetDouble());
  }

  return items;
}

std::string PayoutStatusToString(
    const base::flat_map<std::string, std::string>& payout_status) {
  base::Value::Dict dict;
  for (const auto& status : payout_status) {
    dict.Set(status.first, status.second);
  }

  std::string payout_status_string;
  base::JSONWriter::Write(dict, &payout_status_string);

  return payout_status_string;
}

base::flat_map<std::string, std::string> StringToPayoutStatus(
    const std::string& payout_status_string) {
  auto json = base::JSONReader::Read(payout_status_string);
  if (!json || !json->is_dict()) {
    return {};
  }

  auto& dict = json->GetDict();
  base::flat_map<std::string, std::string> payout_status;
  for (const auto [key, value] : dict) {
    if (!value.is_string()) {
      continue;
    }
    payout_status.emplace(key, value.GetString());
  }

  return payout_status;
}

base::Value WalletProviderRegionsToValue(
    const base::flat_map<std::string, mojom::RegionsPtr>&
        wallet_provider_regions) {
  base::Value::Dict wallet_provider_regions_dict;

  for (const auto& [wallet_provider, regions] : wallet_provider_regions) {
    base::Value::List allow;
    for (const auto& country : regions->allow) {
      allow.Append(country);
    }

    base::Value::List block;
    for (const auto& country : regions->block) {
      block.Append(country);
    }

    base::Value::Dict regions_dict;
    regions_dict.Set("allow", std::move(allow));
    regions_dict.Set("block", std::move(block));

    wallet_provider_regions_dict.Set(wallet_provider, std::move(regions_dict));
  }

  return base::Value(std::move(wallet_provider_regions_dict));
}

base::flat_map<std::string, mojom::RegionsPtr> ValueToWalletProviderRegions(
    const base::Value& value) {
  if (!value.is_dict()) {
    BLOG(0, "Failed to parse JSON!");
    return {};
  }

  auto wallet_provider_regions =
      endpoints::GetWalletProviderRegions(value.GetDict());
  if (!wallet_provider_regions) {
    BLOG(0, "Failed to parse JSON!");
    return {};
  }

  return std::move(*wallet_provider_regions);
}

}  // namespace

namespace state {

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
    amount = GetAutoContributeChoice();
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

void State::SetRewardsParameters(const mojom::RewardsParameters& parameters) {
  engine_->SetState(kParametersRate, parameters.rate);
  engine_->SetState(kParametersAutoContributeChoice,
                    parameters.auto_contribute_choice);
  engine_->SetState(kParametersAutoContributeChoices,
                    VectorDoubleToString(parameters.auto_contribute_choices));
  engine_->SetState(kParametersTipChoices,
                    VectorDoubleToString(parameters.tip_choices));
  engine_->SetState(kParametersMonthlyTipChoices,
                    VectorDoubleToString(parameters.monthly_tip_choices));
  engine_->SetState(kParametersPayoutStatus,
                    PayoutStatusToString(parameters.payout_status));
  engine_->SetState(
      kParametersWalletProviderRegions,
      WalletProviderRegionsToValue(parameters.wallet_provider_regions));
  engine_->SetState(kParametersVBatDeadline, parameters.vbat_deadline);
  engine_->SetState(kParametersVBatExpired, parameters.vbat_expired);
}

mojom::RewardsParametersPtr State::GetRewardsParameters() {
  auto parameters = mojom::RewardsParameters::New();
  parameters->rate = GetRate();
  parameters->auto_contribute_choice = GetAutoContributeChoice();
  parameters->auto_contribute_choices = GetAutoContributeChoices();
  parameters->tip_choices = GetTipChoices();
  parameters->monthly_tip_choices = GetMonthlyTipChoices();
  parameters->payout_status = GetPayoutStatus();
  parameters->wallet_provider_regions = GetWalletProviderRegions();
  parameters->vbat_deadline = GetVBatDeadline();
  parameters->vbat_expired = GetVBatExpired();

  return parameters;
}

double State::GetRate() {
  return engine_->GetState<double>(kParametersRate);
}

double State::GetAutoContributeChoice() {
  return engine_->GetState<double>(kParametersAutoContributeChoice);
}

std::vector<double> State::GetAutoContributeChoices() {
  const std::string amounts_string =
      engine_->GetState<std::string>(kParametersAutoContributeChoices);
  std::vector<double> amounts = StringToVectorDouble(amounts_string);

  const double current_amount = GetAutoContributionAmount();
  if (!base::Contains(amounts, current_amount)) {
    amounts.push_back(current_amount);
    std::sort(amounts.begin(), amounts.end());
  }

  return amounts;
}

std::vector<double> State::GetTipChoices() {
  return StringToVectorDouble(
      engine_->GetState<std::string>(kParametersTipChoices));
}

std::vector<double> State::GetMonthlyTipChoices() {
  return StringToVectorDouble(
      engine_->GetState<std::string>(kParametersMonthlyTipChoices));
}

base::flat_map<std::string, std::string> State::GetPayoutStatus() {
  return StringToPayoutStatus(
      engine_->GetState<std::string>(kParametersPayoutStatus));
}

base::flat_map<std::string, mojom::RegionsPtr>
State::GetWalletProviderRegions() {
  return ValueToWalletProviderRegions(
      engine_->GetState<base::Value>(kParametersWalletProviderRegions));
}

base::Time State::GetVBatDeadline() {
  return engine_->GetState<base::Time>(kParametersVBatDeadline);
}

bool State::GetVBatExpired() {
  return engine_->GetState<bool>(kParametersVBatExpired);
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
    BLOG(0, "Base64 decoding failed for " << key);
    return {};
  }

  auto decrypted = engine_->DecryptString(value);
  if (!decrypted) {
    BLOG(0, "Decryption failed for " << key);
    return {};
  }

  return *decrypted;
}

bool State::SetEncryptedString(const std::string& key,
                               const std::string& value) {
  auto encrypted = engine_->EncryptString(value);
  if (!encrypted) {
    BLOG(0, "Encryption failed for " << key);
    return false;
  }

  std::string base64_string;
  base::Base64Encode(*encrypted, &base64_string);

  engine_->SetState(key, std::move(base64_string));
  return true;
}

}  // namespace state
}  // namespace brave_rewards::internal
