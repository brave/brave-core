/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/state/state_util.h"

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

}  // namespace braveledger_state
