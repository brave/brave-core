/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/state/state_keys.h"
#include "bat/ledger/internal/state/state_migration_v1.h"

using std::placeholders::_1;

namespace braveledger_state {

StateMigrationV1::StateMigrationV1(bat_ledger::LedgerImpl* ledger) :
    ledger_(ledger) {
}

StateMigrationV1::~StateMigrationV1() = default;

void StateMigrationV1::Migrate(ledger::ResultCallback callback) {
  legacy_publisher_ =
      std::make_unique<braveledger_publisher::LegacyPublisherState>(ledger_);

  auto load_callback = std::bind(&StateMigrationV1::OnLoadState,
      this,
      _1,
      callback);

  legacy_publisher_->Load(load_callback);
}

void StateMigrationV1::OnLoadState(
    const ledger::Result result,
    ledger::ResultCallback callback) {
  if (result == ledger::Result::NO_PUBLISHER_STATE) {
    BLOG(1, "No publisher state");
    ledger_->publisher()->CalcScoreConsts(
        ledger_->ledger_client()->GetIntegerState(ledger::kStateMinVisitTime));

    callback(ledger::Result::LEDGER_OK);
    return;
  }

  if (result != ledger::Result::LEDGER_OK) {
    ledger_->publisher()->CalcScoreConsts(
        ledger_->ledger_client()->GetIntegerState(ledger::kStateMinVisitTime));

    BLOG(0, "Failed to load publisher state file, setting default values");
    callback(ledger::Result::LEDGER_OK);
    return;
  }

  ledger_->ledger_client()->SetIntegerState(
      ledger::kStateMinVisitTime,
      static_cast<int>(legacy_publisher_->GetPublisherMinVisitTime()));
  ledger_->publisher()->CalcScoreConsts(
      ledger_->ledger_client()->GetIntegerState(ledger::kStateMinVisitTime));

  ledger_->ledger_client()->SetIntegerState(
      ledger::kStateMinVisits,
      static_cast<int>(legacy_publisher_->GetPublisherMinVisits()));

  ledger_->ledger_client()->SetBooleanState(
      ledger::kStateAllowNonVerified,
      legacy_publisher_->GetPublisherAllowNonVerified());

  ledger_->ledger_client()->SetBooleanState(
      ledger::kStateAllowVideoContribution,
      legacy_publisher_->GetPublisherAllowVideos());

  ledger::BalanceReportInfoList reports;
  legacy_publisher_->GetAllBalanceReports(&reports);
  if (!reports.empty()) {
    auto save_callback = std::bind(&StateMigrationV1::BalanceReportsSaved,
      this,
      _1,
      callback);

    ledger_->database()->SaveBalanceReportInfoList(
        std::move(reports),
        save_callback);
    return;
  }

  SaveProcessedPublishers(callback);
}

void StateMigrationV1::BalanceReportsSaved(
    const ledger::Result result,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Balance report save failed");
    callback(result);
    return;
  }

  SaveProcessedPublishers(callback);
}

void StateMigrationV1::SaveProcessedPublishers(
    ledger::ResultCallback callback) {
  auto save_callback = std::bind(&StateMigrationV1::ProcessedPublisherSaved,
    this,
    _1,
    callback);

  ledger_->database()->SaveProcessedPublisherList(
      legacy_publisher_->GetAlreadyProcessedPublishers(),
      save_callback);
}

void StateMigrationV1::ProcessedPublisherSaved(
    const ledger::Result result,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(0, "Processed publisher save failed");
    callback(result);
    return;
  }

  callback(ledger::Result::LEDGER_OK);
}

}  // namespace braveledger_state
