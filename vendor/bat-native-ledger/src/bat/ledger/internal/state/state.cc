/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/values.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/state/state_migration.h"
#include "bat/ledger/internal/static_values.h"

namespace {

std::string VectorDoubleToString(const std::vector<double>& items) {
  base::Value list(base::Value::Type::LIST);
  for (const auto& item : items) {
    list.Append(base::Value(item));
  }

  std::string items_string;
  base::JSONWriter::Write(list, &items_string);

  return items_string;
}

std::vector<double> StringToVectorDouble(const std::string& items_string) {
  base::Optional<base::Value> list = base::JSONReader::Read(items_string);
  if (!list || !list->is_list()) {
    return {};
  }

  base::ListValue* list_value = nullptr;
  if (!list->GetAsList(&list_value)) {
    return {};
  }

  std::vector<double> items;
  for (auto& item : list_value->GetList()) {
    if (!item.is_double()) {
      continue;
    }
    items.push_back(item.GetDouble());
  }

  return items;
}

std::string ConvertInlineTipPlatformToKey(
    const ledger::InlineTipsPlatforms platform) {
  switch (platform) {
    case ledger::InlineTipsPlatforms::REDDIT: {
      return ledger::kStateInlineTipRedditEnabled;
    }
    case ledger::InlineTipsPlatforms::TWITTER: {
      return ledger::kStateInlineTipTwitterEnabled;
    }
    case ledger::InlineTipsPlatforms::GITHUB: {
      return ledger::kStateInlineTipGithubEnabled;
    }
    case ledger::InlineTipsPlatforms::NONE: {
      NOTREACHED();
      return "";
    }
  }
}

}  // namespace

namespace braveledger_state {

State::State(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger),
    migration_(std::make_unique<StateMigration>(ledger)) {
  DCHECK(ledger_);
}

State::~State() = default;

void State::Initialize(ledger::ResultCallback callback) {
  migration_->Migrate(callback);
}

void State::SetVersion(const int version) {
  ledger_->database()->SaveEventLog(
      ledger::kStateVersion,
      std::to_string(version));
  ledger_->ledger_client()->SetIntegerState(ledger::kStateVersion,  version);
}

int State::GetVersion() {
  return ledger_->ledger_client()->GetIntegerState(ledger::kStateVersion);
}

void State::SetPublisherMinVisitTime(const int duration) {
  ledger_->database()->SaveEventLog(
      ledger::kStateMinVisitTime,
      std::to_string(duration));
  ledger_->ledger_client()->SetIntegerState(
      ledger::kStateMinVisitTime,
      duration);
  ledger_->publisher()->CalcScoreConsts(duration);
  ledger_->publisher()->SynopsisNormalizer();
}

int State::GetPublisherMinVisitTime() {
  return ledger_->ledger_client()->GetIntegerState(ledger::kStateMinVisitTime);
}

void State::SetPublisherMinVisits(const int visits) {
  ledger_->database()->SaveEventLog(
      ledger::kStateMinVisits,
      std::to_string(visits));
  ledger_->ledger_client()->SetIntegerState(ledger::kStateMinVisits, visits);
  ledger_->publisher()->SynopsisNormalizer();
}

int State::GetPublisherMinVisits() {
  return ledger_->ledger_client()->GetIntegerState(ledger::kStateMinVisits);
}

void State::SetPublisherAllowNonVerified(const bool allow) {
  ledger_->database()->SaveEventLog(
      ledger::kStateAllowNonVerified,
      std::to_string(allow));
  ledger_->ledger_client()->SetBooleanState(
      ledger::kStateAllowNonVerified,
      allow);
  ledger_->publisher()->SynopsisNormalizer();
}

bool State::GetPublisherAllowNonVerified() {
  return ledger_->ledger_client()->GetBooleanState(
      ledger::kStateAllowNonVerified);
}

void State::SetPublisherAllowVideos(const bool allow) {
  ledger_->database()->SaveEventLog(
      ledger::kStateAllowVideoContribution,
      std::to_string(allow));
  ledger_->ledger_client()->SetBooleanState(
      ledger::kStateAllowVideoContribution,
      allow);
  ledger_->publisher()->SynopsisNormalizer();
}

bool State::GetPublisherAllowVideos() {
  return ledger_->ledger_client()->GetBooleanState(
      ledger::kStateAllowVideoContribution);
}

void State::SetScoreValues(double a, double b) {
  ledger_->database()->SaveEventLog(ledger::kStateScoreA, std::to_string(a));
  ledger_->database()->SaveEventLog(ledger::kStateScoreB, std::to_string(b));
  ledger_->ledger_client()->SetDoubleState(ledger::kStateScoreA, a);
  ledger_->ledger_client()->SetDoubleState(ledger::kStateScoreB, b);
}

void State::GetScoreValues(double* a, double* b) {
  DCHECK(a && b);
  *a = ledger_->ledger_client()->GetDoubleState(ledger::kStateScoreA);
  *b = ledger_->ledger_client()->GetDoubleState(ledger::kStateScoreB);
}

void State::SetRewardsMainEnabled(bool enabled) {
  ledger_->database()->SaveEventLog(
      ledger::kStateEnabled,
      std::to_string(enabled));
  ledger_->ledger_client()->SetBooleanState(ledger::kStateEnabled, enabled);
}

bool State::GetRewardsMainEnabled() {
  return ledger_->ledger_client()->GetBooleanState(ledger::kStateEnabled);
}

void State::SetAutoContributeEnabled(bool enabled) {
  ledger_->database()->SaveEventLog(
      ledger::kStateAutoContributeEnabled,
      std::to_string(enabled));
  ledger_->ledger_client()->SetBooleanState(
      ledger::kStateAutoContributeEnabled,
      enabled);
}

bool State::GetAutoContributeEnabled() {
  return ledger_->ledger_client()->GetBooleanState(
      ledger::kStateAutoContributeEnabled);
}

void State::SetAutoContributionAmount(const double amount) {
  ledger_->database()->SaveEventLog(
      ledger::kStateAutoContributeAmount,
      std::to_string(amount));
  ledger_->ledger_client()->SetDoubleState(
      ledger::kStateAutoContributeAmount,
      amount);
}

double State::GetAutoContributionAmount() {
  double amount =
      ledger_->ledger_client()->GetDoubleState(
          ledger::kStateAutoContributeAmount);
  if (amount == 0.0) {
    amount = GetAutoContributeChoice();
  }

  return amount;
}

uint64_t State::GetReconcileStamp() {
  auto stamp = ledger_->ledger_client()->GetUint64State(
      ledger::kStateNextReconcileStamp);
  if (stamp == 0) {
    ResetReconcileStamp();
    stamp = ledger_->ledger_client()->GetUint64State(
        ledger::kStateNextReconcileStamp);
  }

  return stamp;
}

void State::SetReconcileStamp(const int reconcile_interval) {
  uint64_t reconcile_stamp = braveledger_time_util::GetCurrentTimeStamp();
  if (reconcile_interval > 0) {
    reconcile_stamp += reconcile_interval * 60;
  } else {
    reconcile_stamp += braveledger_ledger::_reconcile_default_interval;
  }

  ledger_->database()->SaveEventLog(
      ledger::kStateNextReconcileStamp,
      std::to_string(reconcile_stamp));
  ledger_->ledger_client()->SetUint64State(
      ledger::kStateNextReconcileStamp,
      reconcile_stamp);
  ledger_->ledger_client()->ReconcileStampReset();
}
void State::ResetReconcileStamp() {
  SetReconcileStamp(ledger::reconcile_interval);
}

uint64_t State::GetCreationStamp() {
  return ledger_->ledger_client()->GetUint64State(ledger::kStateCreationStamp);
}

void State::SetCreationStamp(const uint64_t stamp) {
  ledger_->database()->SaveEventLog(
      ledger::kStateCreationStamp,
      std::to_string(stamp));
  ledger_->ledger_client()->SetUint64State(ledger::kStateCreationStamp, stamp);
}

std::vector<uint8_t> State::GetRecoverySeed() {
  const std::string& seed = ledger_->ledger_client()->GetStringState(
      ledger::kStateRecoverySeed);
  std::string decoded_seed;
  if (!base::Base64Decode(seed, &decoded_seed)) {
    BLOG(0, "Problem decoding recovery seed");
    NOTREACHED();
    return {};
  }

  std::vector<uint8_t> vector_seed;
  vector_seed.assign(decoded_seed.begin(), decoded_seed.end());
  return vector_seed;
}

void State::SetRecoverySeed(const std::vector<uint8_t>& seed) {
  const std::string seed_string = base::Base64Encode(seed);
  std::string event_string;
  if (seed.size() > 1) {
    event_string = std::to_string(seed[0]+seed[1]);
  }
  ledger_->database()->SaveEventLog(ledger::kStateRecoverySeed, event_string);
  ledger_->ledger_client()->SetStringState(
      ledger::kStateRecoverySeed,
      seed_string);
}

std::string State::GetPaymentId() {
  return ledger_->ledger_client()->GetStringState(ledger::kStatePaymentId);
}

void State::SetPaymentId(const std::string& id) {
  ledger_->database()->SaveEventLog(ledger::kStatePaymentId, id);
  ledger_->ledger_client()->SetStringState(ledger::kStatePaymentId, id);
}

bool State::GetInlineTippingPlatformEnabled(
    const ledger::InlineTipsPlatforms platform) {
  return ledger_->ledger_client()->GetBooleanState(
      ConvertInlineTipPlatformToKey(platform));
}

void State::SetInlineTippingPlatformEnabled(
    const ledger::InlineTipsPlatforms platform,
    const bool enabled) {
  const std::string platform_string = ConvertInlineTipPlatformToKey(platform);
  ledger_->database()->SaveEventLog(platform_string, std::to_string(enabled));
  ledger_->ledger_client()->SetBooleanState(
      platform_string,
      enabled);
}

void State::SetRewardsParameters(const ledger::RewardsParameters& parameters) {
  ledger_->ledger_client()->SetDoubleState(
      ledger::kStateParametersRate,
      parameters.rate);
  ledger_->ledger_client()->SetDoubleState(
      ledger::kStateParametersAutoContributeChoice,
      parameters.auto_contribute_choice);
  ledger_->ledger_client()->SetStringState(
      ledger::kStateParametersAutoContributeChoices,
      VectorDoubleToString(parameters.auto_contribute_choices));
  ledger_->ledger_client()->SetStringState(
      ledger::kStateParametersTipChoices,
      VectorDoubleToString(parameters.tip_choices));
  ledger_->ledger_client()->SetStringState(
      ledger::kStateParametersMonthlyTipChoices,
      VectorDoubleToString(parameters.monthly_tip_choices));
}

ledger::RewardsParametersPtr State::GetRewardsParameters() {
  auto parameters = ledger::RewardsParameters::New();
  parameters->rate = GetRate();
  parameters->auto_contribute_choice = GetAutoContributeChoice();
  parameters->auto_contribute_choices = GetAutoContributeChoices();
  parameters->tip_choices = GetTipChoices();
  parameters->monthly_tip_choices = GetMonthlyTipChoices();

  return parameters;
}

double State::GetRate() {
  return ledger_->ledger_client()->GetDoubleState(ledger::kStateParametersRate);
}

double State::GetAutoContributeChoice() {
  return ledger_->ledger_client()->GetDoubleState(
      ledger::kStateParametersAutoContributeChoice);
}

std::vector<double> State::GetAutoContributeChoices() {
  const std::string amounts_string = ledger_->ledger_client()->GetStringState(
      ledger::kStateParametersAutoContributeChoices);
  std::vector<double> amounts = StringToVectorDouble(amounts_string);

  const double current_amount = GetAutoContributionAmount();
  auto contains_amount = std::find(
      amounts.begin(),
      amounts.end(),
      current_amount);

  if (contains_amount == amounts.end()) {
    amounts.push_back(current_amount);
    std::sort(amounts.begin(), amounts.end());
    ledger_->ledger_client()->SetStringState(
      ledger::kStateParametersAutoContributeChoices,
      VectorDoubleToString(amounts));
  }

  return amounts;
}

std::vector<double> State::GetTipChoices() {
  return StringToVectorDouble(ledger_->ledger_client()->GetStringState(
      ledger::kStateParametersTipChoices));
}

std::vector<double> State::GetMonthlyTipChoices() {
  return StringToVectorDouble(ledger_->ledger_client()->GetStringState(
      ledger::kStateParametersMonthlyTipChoices));
}

void State::SetFetchOldBalanceEnabled(bool enabled) {
  ledger_->database()->SaveEventLog(
      ledger::kStateFetchOldBalance,
      std::to_string(enabled));
  ledger_->ledger_client()->SetBooleanState(
      ledger::kStateFetchOldBalance,
      enabled);
}

bool State::GetFetchOldBalanceEnabled() {
  return ledger_->ledger_client()->GetBooleanState(
      ledger::kStateFetchOldBalance);
}

void State::SetEmptyBalanceChecked(const bool checked) {
  ledger_->database()->SaveEventLog(
      ledger::kStateEmptyBalanceChecked,
      std::to_string(checked));
  ledger_->ledger_client()->SetBooleanState(
      ledger::kStateEmptyBalanceChecked,
      checked);
}

bool State::GetEmptyBalanceChecked() {
  return ledger_->ledger_client()->GetBooleanState(
      ledger::kStateEmptyBalanceChecked);
}

void State::SetServerPublisherListStamp(const uint64_t stamp) {
  ledger_->ledger_client()->SetUint64State(
      ledger::kStateServerPublisherListStamp,
      stamp);
}

uint64_t State::GetServerPublisherListStamp() {
  return ledger_->ledger_client()->GetUint64State(
      ledger::kStateServerPublisherListStamp);
}

void State::SetPromotionCorruptedMigrated(const bool migrated) {
  ledger_->database()->SaveEventLog(
      ledger::kStatePromotionCorruptedMigrated,
      std::to_string(migrated));
  ledger_->ledger_client()->SetBooleanState(
      ledger::kStatePromotionCorruptedMigrated,
      migrated);
}

bool State::GetPromotionCorruptedMigrated() {
  return ledger_->ledger_client()->GetBooleanState(
      ledger::kStatePromotionCorruptedMigrated);
}

void State::SetPromotionLastFetchStamp(const uint64_t stamp) {
  ledger_->ledger_client()->SetUint64State(
      ledger::kStatePromotionLastFetchStamp,
      stamp);
}

uint64_t State::GetPromotionLastFetchStamp() {
  return ledger_->ledger_client()->GetUint64State(
      ledger::kStatePromotionLastFetchStamp);
}

void State::SetAnonTransferChecked(const bool checked) {
  ledger_->database()->SaveEventLog(
      ledger::kStateAnonTransferChecked,
      std::to_string(checked));
  ledger_->ledger_client()->SetBooleanState(
      ledger::kStateAnonTransferChecked,
      checked);
}

bool State::GetAnonTransferChecked() {
  return ledger_->ledger_client()->GetBooleanState(
      ledger::kStateAnonTransferChecked);
}

}  // namespace braveledger_state
