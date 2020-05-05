/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <utility>
#include <vector>

#include "base/strings/stringprintf.h"
#include "bat/ledger/internal/ledger_impl.h"
#include "bat/ledger/internal/legacy/publisher_settings_state.h"
#include "bat/ledger/internal/legacy/publisher_state.h"
#include "bat/ledger/internal/legacy/report_balance_properties.h"
#include "bat/ledger/internal/publisher/publisher.h"
#include "bat/ledger/internal/static_values.h"

using std::placeholders::_1;

namespace braveledger_publisher {

LegacyPublisherState::LegacyPublisherState(
    bat_ledger::LedgerImpl* ledger,
    Publisher* publisher):
  ledger_(ledger),
  publisher_(publisher),
  state_(new ledger::PublisherSettingsProperties) {
}

LegacyPublisherState::~LegacyPublisherState() = default;

std::string LegacyPublisherState::GetBalanceReportName(
    const ledger::ActivityMonth month,
    int year) {
  return base::StringPrintf("%d_%d", year, month);
}

// In seconds
void LegacyPublisherState::SetPublisherMinVisitTime(const uint64_t& duration) {
  state_->min_page_time_before_logging_a_visit = duration;
  publisher_->CalcScoreConsts(duration);
  publisher_->SynopsisNormalizer();
  SaveState();
}

void LegacyPublisherState::SetPublisherMinVisits(const unsigned int visits) {
  state_->min_visits_for_publisher_relevancy = visits;
  publisher_->SynopsisNormalizer();
  SaveState();
}

void LegacyPublisherState::SetPublisherAllowNonVerified(const bool allow) {
  state_->allow_non_verified_sites_in_list = allow;
  publisher_->SynopsisNormalizer();
  SaveState();
}

void LegacyPublisherState::SetPublisherAllowVideos(const bool allow) {
  state_->allow_contribution_to_videos = allow;
  publisher_->SynopsisNormalizer();
  SaveState();
}

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

bool LegacyPublisherState::GetMigrateScore() const {
  return state_->migrate_score_2;
}

void LegacyPublisherState::SetMigrateScore(bool value) {
  state_->migrate_score_2 = value;
  SaveState();
}

void LegacyPublisherState::ClearAllBalanceReports() {
  if (state_->monthly_balances.empty()) {
    return;
  }

  state_->monthly_balances.clear();
  SaveState();
}

void LegacyPublisherState::SetBalanceReport(
    ledger::ActivityMonth month,
    int year,
    const ledger::BalanceReportInfo& report_info) {
  ledger::ReportBalanceProperties report_balance;
  report_balance.grants = report_info.grants;
  report_balance.ad_earnings = report_info.earning_from_ads;
  report_balance.recurring_donations = report_info.recurring_donation;
  report_balance.one_time_donations = report_info.one_time_donation;
  report_balance.auto_contributions = report_info.auto_contribute;

  state_->monthly_balances[GetBalanceReportName(month, year)] = report_balance;
  SaveState();
}

void LegacyPublisherState::GetBalanceReport(
    const ledger::ActivityMonth month,
    const int year,
    ledger::GetBalanceReportCallback callback) {
  ledger::BalanceReportInfo info;
  const auto result = GetBalanceReportInternal(month, year, &info);
  callback(result, info.Clone());
}

ledger::Result LegacyPublisherState::GetBalanceReportInternal(
    const ledger::ActivityMonth month,
    const int year,
    ledger::BalanceReportInfo* report_info) {
  if (!report_info) {
    return ledger::Result::LEDGER_ERROR;
  }

  const std::string name = GetBalanceReportName(month, year);
  auto iter = state_->monthly_balances.find(name);

  if (iter == state_->monthly_balances.end()) {
    ledger::BalanceReportInfo new_report_info;
    new_report_info.grants = 0.0;
    new_report_info.earning_from_ads = 0.0;
    new_report_info.auto_contribute = 0.0;
    new_report_info.recurring_donation = 0.0;
    new_report_info.one_time_donation = 0.0;

    SetBalanceReport(month, year, new_report_info);
    ledger::Result result = GetBalanceReportInternal(month, year, report_info);
    if (result == ledger::Result::LEDGER_OK) {
      iter = state_->monthly_balances.find(name);
    } else {
      return ledger::Result::LEDGER_ERROR;
    }
  }

  report_info->grants = iter->second.grants;
  report_info->earning_from_ads = iter->second.ad_earnings;
  report_info->auto_contribute = iter->second.auto_contributions;
  report_info->recurring_donation = iter->second.recurring_donations;
  report_info->one_time_donation = iter->second.one_time_donations;

  return ledger::Result::LEDGER_OK;
}

std::map<std::string, ledger::BalanceReportInfoPtr>
LegacyPublisherState::GetAllBalanceReports() {
  std::map<std::string, ledger::BalanceReportInfoPtr> newReports;
  for (auto const& report : state_->monthly_balances) {
    ledger::BalanceReportInfoPtr newReport = ledger::BalanceReportInfo::New();
    const ledger::ReportBalanceProperties oldReport = report.second;
    newReport->grants = oldReport.grants;
    newReport->earning_from_ads = oldReport.ad_earnings;
    newReport->auto_contribute = oldReport.auto_contributions;
    newReport->recurring_donation = oldReport.recurring_donations;
    newReport->one_time_donation = oldReport.one_time_donations;

    newReports[report.first] = std::move(newReport);
  }

  return newReports;
}

void LegacyPublisherState::SaveState() {
  const ledger::PublisherSettingsState publisher_settings_state;
  const std::string data = publisher_settings_state.ToJson(*state_);

  auto save_callback = std::bind(&LegacyPublisherState::OnPublisherStateSaved,
      this,
      _1);

  ledger_->SavePublisherState(data, save_callback);
}

void LegacyPublisherState::OnPublisherStateSaved(const ledger::Result result) {
  if (result != ledger::Result::LEDGER_OK) {
    BLOG(ledger_, ledger::LogLevel::LOG_ERROR) <<
      "Could not save publisher state";
    // TODO(anyone) error handling
    return;
  }
}

bool LegacyPublisherState::LoadState(const std::string& data) {
  ledger::PublisherSettingsProperties state;
  const ledger::PublisherSettingsState publisher_settings_state;
  if (!publisher_settings_state.FromJson(data.c_str(), &state)) {
    return false;
  }

  state_.reset(new ledger::PublisherSettingsProperties(state));
  publisher_->CalcScoreConsts(state_->min_page_time_before_logging_a_visit);
  return true;
}

void LegacyPublisherState::SetBalanceReportItem(
    const ledger::ActivityMonth month,
    const int year,
    const ledger::ReportType type,
    const double amount) {
  ledger::BalanceReportInfo report_info;
  GetBalanceReportInternal(month, year, &report_info);

  switch (type) {
    case ledger::ReportType::GRANT_UGP:
      report_info.grants = report_info.grants + amount;
      break;
    case ledger::ReportType::GRANT_AD:
      report_info.earning_from_ads = report_info.earning_from_ads + amount;
      break;
    case ledger::ReportType::AUTO_CONTRIBUTION:
      report_info.auto_contribute = report_info.auto_contribute + amount;
      break;
    case ledger::ReportType::TIP:
      report_info.one_time_donation = report_info.one_time_donation + amount;
      break;
    case ledger::ReportType::TIP_RECURRING:
      report_info.recurring_donation = report_info.recurring_donation + amount;
      break;
    default:
      break;
  }

  SetBalanceReport(month, year, report_info);
}

void LegacyPublisherState::SavePublisherProcessed(
    const std::string& publisher_key) {
  const std::vector<std::string> list = state_->processed_pending_publishers;
  if (std::find(list.begin(), list.end(), publisher_key) == list.end()) {
    state_->processed_pending_publishers.push_back(publisher_key);
  }
  SaveState();
}

bool LegacyPublisherState::WasPublisherAlreadyProcessed(
    const std::string& publisher_key) const {
  const std::vector<std::string> list = state_->processed_pending_publishers;
  return std::find(list.begin(), list.end(), publisher_key) != list.end();
}

}  // namespace braveledger_publisher
