/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>

#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/legacy/publisher_settings_state.h"
#include "bat/ledger/internal/legacy/publisher_state.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace braveledger_publisher {

LegacyPublisherState::LegacyPublisherState(bat_ledger::LedgerImpl* ledger) :
  ledger_(ledger),
  state_(new ledger::PublisherSettingsProperties) {
}

LegacyPublisherState::~LegacyPublisherState() = default;

uint64_t LegacyPublisherState::GetPublisherMinVisitTime() const {
  return state_->min_page_time_before_logging_a_visit;
}

unsigned int LegacyPublisherState::GetPublisherMinVisits() const {
  return state_->min_visits_for_publisher_relevancy;
}

bool LegacyPublisherState::GetPublisherAllowNonVerified() const {
  return state_->allow_non_verified_sites_in_list;
}

bool LegacyPublisherState::GetPublisherAllowVideos() const {
  return state_->allow_contribution_to_videos;
}

void LegacyPublisherState::Load(ledger::ResultCallback callback) {
  auto load_callback = std::bind(&LegacyPublisherState::OnLoad,
      this,
      _1,
      _2,
      callback);
  ledger_->ledger_client()->LoadPublisherState(load_callback);
}

void LegacyPublisherState::OnLoad(
    const ledger::Result result,
    const std::string& data,
    ledger::ResultCallback callback) {
  if (result != ledger::Result::LEDGER_OK) {
    callback(result);
    return;
  }

  ledger::PublisherSettingsProperties state;
  const ledger::PublisherSettingsState publisher_settings_state;
  if (!publisher_settings_state.FromJson(data.c_str(), &state)) {
    callback(ledger::Result::LEDGER_ERROR);
    return;
  }

  state_.reset(new ledger::PublisherSettingsProperties(state));
  callback(ledger::Result::LEDGER_OK);
}

std::vector<std::string>
LegacyPublisherState::GetAlreadyProcessedPublishers() const {
  return state_->processed_pending_publishers;
}

void LegacyPublisherState::GetAllBalanceReports(
    ledger::BalanceReportInfoList* reports) {
  DCHECK(reports);

  for (auto const& report : state_->monthly_balances) {
    auto report_ptr = ledger::BalanceReportInfo::New();
    report_ptr->id = report.first;
    report_ptr->grants = report.second.grants;
    report_ptr->earning_from_ads = report.second.ad_earnings;
    report_ptr->auto_contribute = report.second.auto_contributions;
    report_ptr->recurring_donation = report.second.recurring_donations;
    report_ptr->one_time_donation = report.second.one_time_donations;

    reports->push_back(std::move(report_ptr));
  }
}

}  // namespace braveledger_publisher
