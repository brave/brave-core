/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_STATE_STATE_UTIL_H_
#define BRAVELEDGER_STATE_STATE_UTIL_H_

#include <string>

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/mojom_structs.h"

namespace braveledger_state {

void SetVersion(bat_ledger::LedgerImpl* ledger, const int version);

int GetVersion(bat_ledger::LedgerImpl* ledger);

void SetPublisherMinVisitTime(
    bat_ledger::LedgerImpl* ledger,
    const int duration);

int GetPublisherMinVisitTime(bat_ledger::LedgerImpl* ledger);

void SetPublisherMinVisits(bat_ledger::LedgerImpl* ledger, const int visits);

int GetPublisherMinVisits(bat_ledger::LedgerImpl* ledger);

void SetPublisherAllowNonVerified(
    bat_ledger::LedgerImpl* ledger,
    const bool allow);

bool GetPublisherAllowNonVerified(bat_ledger::LedgerImpl* ledger);

void SetPublisherAllowVideos(
    bat_ledger::LedgerImpl* ledger,
    const bool allow);

bool GetPublisherAllowVideos(bat_ledger::LedgerImpl* ledger);

void SetScoreValues(
    bat_ledger::LedgerImpl* ledger,
    double a,
    double b);

void GetScoreValues(
    bat_ledger::LedgerImpl* ledger,
    double* a,
    double* b);

}  // namespace braveledger_state

#endif  // BRAVELEDGER_STATE_STATE_UTIL_H_
