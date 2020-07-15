/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_STATE_STATE_UTIL_H_
#define BRAVELEDGER_STATE_STATE_UTIL_H_

#include <string>
#include <vector>

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

void SetRewardsMainEnabled(bat_ledger::LedgerImpl* ledger, bool enabled);

bool GetRewardsMainEnabled(bat_ledger::LedgerImpl* ledger);

void SetAutoContributeEnabled(bat_ledger::LedgerImpl* ledger, bool enabled);

bool GetAutoContributeEnabled(bat_ledger::LedgerImpl* ledger);

void SetAutoContributionAmount(
    bat_ledger::LedgerImpl* ledger,
    const double amount);

double GetAutoContributionAmount(bat_ledger::LedgerImpl* ledger);

uint64_t GetReconcileStamp(bat_ledger::LedgerImpl* ledger);

void SetReconcileStamp(
    bat_ledger::LedgerImpl* ledger,
    const int reconcile_interval);

uint64_t GetCreationStamp(bat_ledger::LedgerImpl* ledger);

void SetCreationStamp(bat_ledger::LedgerImpl* ledger, const uint64_t stamp);

std::vector<uint8_t> GetRecoverySeed(bat_ledger::LedgerImpl* ledger);

void SetRecoverySeed(
    bat_ledger::LedgerImpl* ledger,
    const std::vector<uint8_t>& seed);

std::string GetPaymentId(bat_ledger::LedgerImpl* ledger);

void SetPaymentId(bat_ledger::LedgerImpl* ledger, const std::string& id);

std::string ConvertInlineTipPlatformToString(
    const ledger::InlineTipsPlatforms platform);

bool GetInlineTippingPlatformEnabled(
    bat_ledger::LedgerImpl* ledger,
    const ledger::InlineTipsPlatforms platform);

void SetInlineTippingPlatformEnabled(
    bat_ledger::LedgerImpl* ledger,
    const ledger::InlineTipsPlatforms platform,
    const bool enabled);

void SetRewardsParameters(
    bat_ledger::LedgerImpl* ledger,
    const ledger::RewardsParameters& parameters);

ledger::RewardsParametersPtr GetRewardsParameters(
    bat_ledger::LedgerImpl* ledger);

double GetRate(bat_ledger::LedgerImpl* ledger);

double GetAutoContributeChoice(bat_ledger::LedgerImpl* ledger);

std::vector<double> GetAutoContributeChoices(bat_ledger::LedgerImpl* ledger);

std::vector<double> GetTipChoices(bat_ledger::LedgerImpl* ledger);

std::vector<double> GetMonthlyTipChoices(bat_ledger::LedgerImpl* ledger);

void SetFetchOldBalanceEnabled(bat_ledger::LedgerImpl* ledger, bool enabled);

bool GetFetchOldBalanceEnabled(bat_ledger::LedgerImpl* ledger);

}  // namespace braveledger_state

#endif  // BRAVELEDGER_STATE_STATE_UTIL_H_
