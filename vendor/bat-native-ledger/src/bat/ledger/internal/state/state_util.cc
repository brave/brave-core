/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>

#include "base/base64.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/state/state_util.h"
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
    items.push_back(item.GetDouble());
  }

  return items;
}

}  // namespace

namespace braveledger_state {

void SetVersion(bat_ledger::LedgerImpl* ledger, const int version) {
  DCHECK(ledger);
  ledger->SetIntegerState(ledger::kStateVersion,  version);
}

int GetVersion(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  return ledger->GetIntegerState(ledger::kStateVersion);
}

void SetPublisherMinVisitTime(
    bat_ledger::LedgerImpl* ledger,
    const int duration) {
  DCHECK(ledger);
  ledger->SetIntegerState(ledger::kStateMinVisitTime, duration);
  ledger->CalcScoreConsts(duration);
  ledger->SynopsisNormalizer();
}

int GetPublisherMinVisitTime(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  return ledger->GetIntegerState(ledger::kStateMinVisitTime);
}

void SetPublisherMinVisits(
    bat_ledger::LedgerImpl* ledger,
    const int visits) {
  DCHECK(ledger);
  ledger->SetIntegerState(ledger::kStateMinVisits, visits);
  ledger->SynopsisNormalizer();
}

int GetPublisherMinVisits(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  return ledger->GetIntegerState(ledger::kStateMinVisits);
}

void SetPublisherAllowNonVerified(
    bat_ledger::LedgerImpl* ledger,
    const bool allow) {
  DCHECK(ledger);
  ledger->SetBooleanState(ledger::kStateAllowNonVerified, allow);
  ledger->SynopsisNormalizer();
}

bool GetPublisherAllowNonVerified(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  return ledger->GetBooleanState(ledger::kStateAllowNonVerified);
}

void SetPublisherAllowVideos(
    bat_ledger::LedgerImpl* ledger,
    const bool allow) {
  DCHECK(ledger);
  ledger->SetBooleanState(ledger::kStateAllowVideoContribution, allow);
  ledger->SynopsisNormalizer();
}

bool GetPublisherAllowVideos(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  return ledger->GetBooleanState(ledger::kStateAllowVideoContribution);
}

void SetScoreValues(
    bat_ledger::LedgerImpl* ledger,
    double a,
    double b) {
  DCHECK(ledger);
  ledger->SetDoubleState(ledger::kStateScoreA, a);
  ledger->SetDoubleState(ledger::kStateScoreB, b);
}

void GetScoreValues(
  bat_ledger::LedgerImpl* ledger,
  double* a,
  double* b) {
  DCHECK(ledger && a && b);
  *a = ledger->GetDoubleState(ledger::kStateScoreA);
  *b = ledger->GetDoubleState(ledger::kStateScoreB);
}

void SetRewardsMainEnabled(bat_ledger::LedgerImpl* ledger, bool enabled) {
  DCHECK(ledger);
  ledger->SetBooleanState(ledger::kStateEnabled, enabled);
}

bool GetRewardsMainEnabled(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  return ledger->GetBooleanState(ledger::kStateEnabled);
}

void SetAutoContributeEnabled(bat_ledger::LedgerImpl* ledger, bool enabled) {
  DCHECK(ledger);
  ledger->SetBooleanState(ledger::kStateAutoContributeEnabled, enabled);
}

bool GetAutoContributeEnabled(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  return ledger->GetBooleanState(ledger::kStateAutoContributeEnabled);
}

void SetAutoContributionAmount(
    bat_ledger::LedgerImpl* ledger,
    const double amount) {
  DCHECK(ledger);
  ledger->SetDoubleState(ledger::kStateAutoContributeAmount, amount);
}

double GetAutoContributionAmount(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  double amount =
      ledger->GetDoubleState(ledger::kStateAutoContributeAmount);
  if (amount == 0.0) {
    amount = GetAutoContributeChoice(ledger);
  }

  return amount;
}

uint64_t GetReconcileStamp(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  return ledger->GetUint64State(ledger::kStateNextReconcileStamp);
}

void SetReconcileStamp(
    bat_ledger::LedgerImpl* ledger,
    const int reconcile_interval) {
  DCHECK(ledger);
  uint64_t reconcile_stamp = braveledger_time_util::GetCurrentTimeStamp();
  if (reconcile_interval > 0) {
    reconcile_stamp += reconcile_interval * 60;
  } else {
    reconcile_stamp += braveledger_ledger::_reconcile_default_interval;
  }

  ledger->SetUint64State(ledger::kStateNextReconcileStamp, reconcile_stamp);
}

uint64_t GetCreationStamp(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  return ledger->GetUint64State(ledger::kStateCreationStamp);
}

void SetCreationStamp(bat_ledger::LedgerImpl* ledger, const uint64_t stamp) {
  DCHECK(ledger);
  ledger->SetUint64State(ledger::kStateCreationStamp, stamp);
}

std::string GetAnonymousCardId(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  return ledger->GetStringState(ledger::kStateAnonymousCardId);
}

void SetAnonymousCardId(
    bat_ledger::LedgerImpl* ledger,
    const std::string& id) {
  DCHECK(ledger);
  ledger->SetStringState(ledger::kStateAnonymousCardId, id);
}

std::vector<uint8_t> GetRecoverySeed(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  const std::string& seed = ledger->GetStringState(ledger::kStateRecoverySeed);
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

void SetRecoverySeed(
    bat_ledger::LedgerImpl* ledger,
    const std::vector<uint8_t>& seed) {
  DCHECK(ledger);
  ledger->SetStringState(ledger::kStateRecoverySeed, base::Base64Encode(seed));
}

std::string GetPaymentId(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  return ledger->GetStringState(ledger::kStatePaymentId);
}

void SetPaymentId(bat_ledger::LedgerImpl* ledger, const std::string& id) {
  DCHECK(ledger);
  ledger->SetStringState(ledger::kStatePaymentId, id);
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

bool GetInlineTippingPlatformEnabled(
    bat_ledger::LedgerImpl* ledger,
    const ledger::InlineTipsPlatforms platform) {
  DCHECK(ledger);
  return ledger->GetBooleanState(ConvertInlineTipPlatformToKey(platform));
}

void SetInlineTippingPlatformEnabled(
    bat_ledger::LedgerImpl* ledger,
    const ledger::InlineTipsPlatforms platform,
    const bool enabled) {
  DCHECK(ledger);
  ledger->SetBooleanState(ConvertInlineTipPlatformToKey(platform), enabled);
}

void SetRewardsParameters(
    bat_ledger::LedgerImpl* ledger,
    const ledger::RewardsParameters& parameters) {
  DCHECK(ledger);
  ledger->SetDoubleState(ledger::kStateParametersRate, parameters.rate);
  ledger->SetDoubleState(
      ledger::kStateParametersAutoContributeChoice,
      parameters.auto_contribute_choice);
  ledger->SetStringState(
      ledger::kStateParametersAutoContributeChoices,
      VectorDoubleToString(parameters.auto_contribute_choices));
  ledger->SetStringState(
      ledger::kStateParametersTipChoices,
      VectorDoubleToString(parameters.tip_choices));
  ledger->SetStringState(
      ledger::kStateParametersMonthlyTipChoices,
      VectorDoubleToString(parameters.monthly_tip_choices));
}

ledger::RewardsParametersPtr GetRewardsParameters(
    bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  auto parameters = ledger::RewardsParameters::New();
  parameters->rate = GetRate(ledger);
  parameters->auto_contribute_choice = GetAutoContributeChoice(ledger);
  parameters->auto_contribute_choices = GetAutoContributeChoices(ledger);
  parameters->tip_choices = GetTipChoices(ledger);
  parameters->monthly_tip_choices = GetMonthlyTipChoices(ledger);

  return parameters;
}

double GetRate(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  return ledger->GetDoubleState(ledger::kStateParametersRate);
}

double GetAutoContributeChoice(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  return ledger->GetDoubleState(ledger::kStateParametersAutoContributeChoice);
}

std::vector<double> GetAutoContributeChoices(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  const std::string amounts_string =
      ledger->GetStringState(ledger::kStateParametersAutoContributeChoices);
  std::vector<double> amounts = StringToVectorDouble(amounts_string);

  const double current_amount = GetAutoContributionAmount(ledger);
  auto contains_amount = std::find(
      amounts.begin(),
      amounts.end(),
      current_amount);

  if (contains_amount == amounts.end()) {
    amounts.push_back(current_amount);
    std::sort(amounts.begin(), amounts.end());
    ledger->SetStringState(
      ledger::kStateParametersAutoContributeChoices,
      VectorDoubleToString(amounts));
  }

  return amounts;
}

std::vector<double> GetTipChoices(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  const std::string amounts_string =
      ledger->GetStringState(ledger::kStateParametersTipChoices);
  return StringToVectorDouble(amounts_string);
}

std::vector<double> GetMonthlyTipChoices(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  const std::string amounts_string =
      ledger->GetStringState(ledger::kStateParametersMonthlyTipChoices);
  return StringToVectorDouble(amounts_string);
}

void SetFetchOldBalanceEnabled(bat_ledger::LedgerImpl* ledger, bool enabled) {
  DCHECK(ledger);
  ledger->SetBooleanState(ledger::kStateFetchOldBalance, enabled);
}

bool GetFetchOldBalanceEnabled(bat_ledger::LedgerImpl* ledger) {
  DCHECK(ledger);
  return ledger->GetBooleanState(ledger::kStateFetchOldBalance);
}

}  // namespace braveledger_state
