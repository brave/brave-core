/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/base64.h"
#include "bat/ledger/internal/common/time_util.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/state/state_util.h"
#include "bat/ledger/internal/static_values.h"

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
  const double amount =
      ledger->GetDoubleState(ledger::kStateAutoContributeAmount);
  if (amount == 0.0) {
    // TODO(nejc): get default value
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

}  // namespace braveledger_state
