/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include <memory>
#include <utility>

#include "brave/components/brave_rewards/core/ledger_impl.h"
#include "brave/components/brave_rewards/core/legacy/publisher_settings_properties.h"
#include "brave/components/brave_rewards/core/legacy/publisher_state.h"

using std::placeholders::_1;
using std::placeholders::_2;

namespace ledger {
namespace publisher {

LegacyPublisherState::LegacyPublisherState(ledger::LedgerImpl* ledger)
    : ledger_(ledger), state_(new ledger::PublisherSettingsProperties) {}

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

void LegacyPublisherState::Load(ledger::LegacyResultCallback callback) {
  ledger_->client()->LoadPublisherState(
      base::BindOnce(&LegacyPublisherState::OnLoad, base::Unretained(this),
                     std::move(callback)));
}

void LegacyPublisherState::OnLoad(ledger::LegacyResultCallback callback,
                                  ledger::mojom::Result result,
                                  const std::string& data) {
  if (result != ledger::mojom::Result::LEDGER_OK) {
    callback(result);
    return;
  }

  ledger::PublisherSettingsProperties state;
  if (!state.FromJson(data)) {
    callback(ledger::mojom::Result::LEDGER_ERROR);
    return;
  }

  state_ = std::make_unique<ledger::PublisherSettingsProperties>(state);
  callback(ledger::mojom::Result::LEDGER_OK);
}

std::vector<std::string> LegacyPublisherState::GetAlreadyProcessedPublishers()
    const {
  return state_->processed_pending_publishers;
}

void LegacyPublisherState::GetAllBalanceReports(
    std::vector<ledger::mojom::BalanceReportInfoPtr>* reports) {
  DCHECK(reports);

  for (auto const& report : state_->monthly_balances) {
    auto report_ptr = ledger::mojom::BalanceReportInfo::New();
    report_ptr->id = report.first;
    report_ptr->grants = report.second.grants;
    report_ptr->earning_from_ads = report.second.ad_earnings;
    report_ptr->auto_contribute = report.second.auto_contributions;
    report_ptr->recurring_donation = report.second.recurring_donations;
    report_ptr->one_time_donation = report.second.one_time_donations;

    reports->push_back(std::move(report_ptr));
  }
}

}  // namespace publisher
}  // namespace ledger
