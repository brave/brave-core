/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat_publishers.h"

#include <cmath>
#include <algorithm>

#include "bat_helper.h"
#include "ledger_impl.h"
#include "rapidjson_bat_helper.h"
#include "static_values.h"

/* foo.bar.example.com
   QLD = 'bar'
   RLD = 'foo.bar'
   SLD = 'example.com'
   TLD = 'com'

  search.yahoo.co.jp
   QLD = 'search'
   RLD = 'search'
   SLD = 'yahoo.co.jp'
   TLD = 'co.jp'
*/

using namespace std::placeholders;

static bool winners_votes_compare(const braveledger_bat_helper::WINNERS_ST& first, const braveledger_bat_helper::WINNERS_ST& second){
    return (first.votes_ < second.votes_);
}

namespace braveledger_bat_publishers {

BatPublishers::BatPublishers(bat_ledger::LedgerImpl* ledger):
  ledger_(ledger),
  state_(new braveledger_bat_helper::PUBLISHER_STATE_ST) {
  calcScoreConsts();
}

BatPublishers::~BatPublishers() {
}

void BatPublishers::calcScoreConsts() {
  //TODO: check Warning	C4244	'=': conversion from 'double' to 'unsigned int', possible loss of data
  a_ = 1.0 / (braveledger_ledger::_d * 2.0) - state_->min_pubslisher_duration_;
  a2_ = a_ * 2;
  a4_ = a2_ * 2;
  b_ = state_->min_pubslisher_duration_ - a_;
  b2_ = b_ * b_;
}

// courtesy of @dimitry-xyz: https://github.com/brave/ledger/issues/2#issuecomment-221752002
double BatPublishers::concaveScore(const uint64_t& duration) {
  return (std::sqrt(b2_ + a4_ * duration) - b_) / (double)a2_;
}

const ledger::PublisherInfo::id_type getPublisherID(
    const ledger::VisitData& visit_data) {
  return visit_data.tld;
}

const ledger::PublisherInfo::id_type getPublisherID(
    const ledger::PaymentData& payment_data) {
  return payment_data.publisher_id;
}

std::string getProviderName(const ledger::PublisherInfo::id_type publisher_id) {
  // TODO - this is for the media stuff
  return "";
}

bool ignoreMinTime(const ledger::PublisherInfo::id_type publisher_id) {
  return !getProviderName(publisher_id).empty();
}

void BatPublishers::AddRecurringPayment(const std::string& publisher_id, const double& value) {
  state_->recurring_donation_[publisher_id] = value;
  saveState();
}

void BatPublishers::MakePayment(const ledger::PaymentData& payment_data) {
  const ledger::PublisherInfo::id_type publisher_id =
    getPublisherID(payment_data);

  std::string publisher_key = GetPublisherKey(payment_data.category,
    payment_data.local_year, payment_data.local_month, publisher_id);
  ledger_->GetPublisherInfo(publisher_key,
    std::bind(&BatPublishers::makePaymentInternal, this,
                  publisher_key, payment_data, _1, _2));
}

void BatPublishers::saveVisit(const ledger::VisitData& visit_data, const uint64_t& duration) {
  const ledger::PublisherInfo::id_type publisher_id =
      getPublisherID(visit_data);

  if (!ignoreMinTime(publisher_id) &&
      duration < state_->min_pubslisher_duration_)
    return;

  std::string publisher_key = GetPublisherKey(ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE,
    visit_data.local_year, visit_data.local_month, publisher_id);
  ledger_->GetPublisherInfo(publisher_key,
      std::bind(&BatPublishers::saveVisitInternal, this,
                    publisher_key, visit_data, duration, _1, _2));
}

std::string BatPublishers::GetPublisherKey(ledger::PUBLISHER_CATEGORY category, const std::string& year,
    ledger::PUBLISHER_MONTH month, const std::string& publisher_id) {
  return std::to_string(category) + "_" +
    year + "_" + std::to_string(month) + "." + publisher_id;
}

std::string BatPublishers::GetBalanceReportName(const std::string& year,
    const ledger::PUBLISHER_MONTH month) {
  return year + "_" + std::to_string(month) + "_balance";
}

void onVisitSavedDummy(ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  // onPublisherInfoUpdated will always be called by LedgerImpl so do nothing
}

void BatPublishers::makePaymentInternal(
      std::string publisher_key,
      ledger::PaymentData payment_data,
      ledger::Result result,
      std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  if (result != ledger::Result::OK) {
    // TODO error handling
    return;
  }

  if (!publisher_info.get()) 
    publisher_info.reset(new ledger::PublisherInfo(getPublisherID(payment_data)));

  publisher_info->key = publisher_key;
  publisher_info->category = payment_data.category;
  publisher_info->month = payment_data.local_month;
  publisher_info->year = payment_data.local_year;
  publisher_info->contributions.push_back(ledger::ContributionInfo(payment_data.value, payment_data.timestamp));

  ledger_->SetPublisherInfo(std::move(publisher_info),
      std::bind(&onVisitSavedDummy, _1, _2));
}

void BatPublishers::saveVisitInternal(
    std::string publisher_key,
    ledger::VisitData visit_data,
    uint64_t duration,
    ledger::Result result,
    std::unique_ptr<ledger::PublisherInfo> publisher_info) {
  if (result != ledger::Result::OK) {
    // TODO error handling
    return;
  }

  if (!publisher_info.get()) 
    publisher_info.reset(new ledger::PublisherInfo(getPublisherID(visit_data)));

  publisher_info->key = publisher_key;
  publisher_info->duration += duration;
  publisher_info->visits += 1;
  publisher_info->category = ledger::PUBLISHER_CATEGORY::AUTO_CONTRIBUTE;
  publisher_info->score += concaveScore(duration);
  publisher_info->month = visit_data.local_month;
  publisher_info->year = visit_data.local_year;

  ledger_->SetPublisherInfo(std::move(publisher_info),
      std::bind(&onVisitSavedDummy, _1, _2));
}

std::unique_ptr<ledger::PublisherInfo> BatPublishers::onPublisherInfoUpdated(
    ledger::Result result, std::unique_ptr<ledger::PublisherInfo> info) {
  if (result != ledger::Result::OK) {
    // TODO error handling
    return info;
  }

  if (!isEligableForContribution(*info)) {
    publishers_.erase(info->id);
    return info;
  }

  const ledger::PublisherInfo::id_type& publisher_id = info->id;

  auto publisher = publishers_.find(info->id);
  if (publisher != publishers_.end()) {
    publishers_[publisher_id] = braveledger_bat_helper::PUBLISHER_ST();
    publishers_[publisher_id].id_ = publisher_id;
  }

  publishers_[publisher_id].duration_ = info->duration;
  publishers_[publisher_id].score_ = info->score;
  publishers_[publisher_id].visits_ = info->visits;
  publishers_[publisher_id].percent_ = info->percent;
  publishers_[publisher_id].weight_ = info->weight;

  synopsisNormalizer();

  return info;
}

void BatPublishers::setPublisherMinVisitTime(const uint64_t& duration) { // In milliseconds
  state_->min_pubslisher_duration_ = duration; //TODO: conversion from 'const uint64_t' to 'unsigned int', possible loss of data
  saveState();
}

void BatPublishers::setPublisherMinVisits(const unsigned int& visits) {
  state_->min_visits_ = visits;
  saveState();
}

void BatPublishers::setPublisherAllowNonVerified(const bool& allow) {
  state_->allow_non_verified_ = allow;
  saveState();
}

uint64_t BatPublishers::getPublisherMinVisitTime() const {
  return state_->min_pubslisher_duration_;
}

unsigned int BatPublishers::getPublisherMinVisits() const {
  return state_->min_visits_;
}

bool BatPublishers::getPublisherAllowNonVerified() const {
  return state_->allow_non_verified_;
}

uint64_t BatPublishers::getLastPublishersListLoadTimestamp() const {
  return state_->pubs_load_timestamp_;
}

void BatPublishers::synopsisNormalizer() {
  LOG(ERROR)<<"BatPublishers::synopsisNormalizer";
  if (publishers_.size() == 0) {
    return;
  }
  double totalScores = 0.0;
  for (std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::const_iterator iter = publishers_.begin(); iter != publishers_.end(); iter++) {
    totalScores += iter->second.score_;
  }
  std::vector<unsigned int> percents;
  std::vector<double> weights;
  std::vector<double> realPercents;
  std::vector<double> roundoffs;
  unsigned int totalPercents = 0;
  for (std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::iterator iter = publishers_.begin(); iter != publishers_.end(); iter++) {
    realPercents.push_back((double)iter->second.score_ / (double)totalScores * 100.0);
    percents.push_back((unsigned int)std::lround(realPercents[realPercents.size() - 1]));
    double roundoff = percents[percents.size() - 1] - realPercents[realPercents.size() - 1];
    if (roundoff < 0.0) {
      roundoff *= -1.0;
    }
    roundoffs.push_back(roundoff);
    totalPercents += percents[percents.size() - 1];
    // TODO make pinned, unpinned publishers
    weights.push_back((double)iter->second.score_ / (double)publishers_.size() * 100.0);
  }
  while (totalPercents != 100) {
    size_t valueToChange = 0;
    double currentRoundOff = 0.0;
    for (size_t i = 0; i < percents.size(); i++) {
      if (0 == i) {
        currentRoundOff = roundoffs[i];
        continue;
      }
      if (roundoffs[i] > currentRoundOff) {
        currentRoundOff = roundoffs[i];
        valueToChange = i;
      }
    }
    if (0 != percents.size()) {
      if (totalPercents > 100) {
        percents[valueToChange] -= 1;
        totalPercents -= 1;
      } else {
        percents[valueToChange] += 1;
        totalPercents += 1;
      }
      roundoffs[valueToChange] = 0;
    }
  }
  size_t currentValue = 0;
  for (std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::iterator iter = publishers_.begin(); iter != publishers_.end(); iter++) {
    iter->second.percent_ = percents[currentValue];
    iter->second.weight_ = weights[currentValue];
    currentValue++;
  }
}

std::vector<braveledger_bat_helper::WINNERS_ST> BatPublishers::winners(const unsigned int& ballots) {
  std::vector<braveledger_bat_helper::WINNERS_ST> res;
  std::vector<braveledger_bat_helper::PUBLISHER_ST> top = topN();
  unsigned int totalVotes = 0;
  std::vector<unsigned int> votes;
  // TODO there is underscore.shuffle
  for (size_t i = 0; i < top.size(); i++) {
    LOG(ERROR) << "!!!name == " << top[i].id_ << ", score == " << top[i].score_;
    if (top[i].percent_ <= 0) {
      continue;
    }
    braveledger_bat_helper::WINNERS_ST winner;
    winner.votes_ = (unsigned int)std::lround((double)top[i].percent_ * (double)ballots / 100.0);
    totalVotes += winner.votes_;
    winner.publisher_data_ = top[i];
    res.push_back(winner);
  }
  if (res.size()) {
    while (totalVotes > ballots) {
      std::vector<braveledger_bat_helper::WINNERS_ST>::iterator max = std::max_element(res.begin(), res.end(), winners_votes_compare);
      (max->votes_)--;
      totalVotes--;
    }
  }

  return res;
}

std::vector<braveledger_bat_helper::PUBLISHER_ST> BatPublishers::topN() {
  std::vector<braveledger_bat_helper::PUBLISHER_ST> res;

  for (std::map<std::string, braveledger_bat_helper::PUBLISHER_ST>::const_iterator iter = publishers_.begin(); iter != publishers_.end(); iter++)
    res.push_back(iter->second);

  std::sort(res.begin(), res.end());

  return res;
}

bool BatPublishers::isVerified(const ledger::PublisherInfo& publisher_id) {
  // TODO - implement bloom filter
  return true;
}

bool BatPublishers::isEligableForContribution(const ledger::PublisherInfo& info) {

  if (info.excluded || (!state_->allow_non_verified_ && !isVerified(info.id)))
    return false;

  return info.score > 0 &&
    info.duration >= state_->min_pubslisher_duration_ &&
    info.visits >= state_->min_visits_;

}

void BatPublishers::setBalanceReport(const std::string& year,
    ledger::PUBLISHER_MONTH month, const ledger::BalanceReportInfo& report_info) {
  braveledger_bat_helper::REPORT_BALANCE_ST report_balance;
  report_balance.opening_balance_ = report_info.opening_balance_;
  report_balance.closing_balance_ = report_info.closing_balance_;
  report_balance.grants_ = report_info.grants_;
  report_balance.earning_from_ads_ = report_info.earning_from_ads_;
  report_balance.auto_contribute_ = report_info.auto_contribute_;
  report_balance.recurring_donation_ = report_info.recurring_donation_;
  report_balance.one_time_donation_ = report_info.one_time_donation_;

  state_->monthly_balances_[GetBalanceReportName(year, month)] = report_balance;
  saveState();
}

bool BatPublishers::getBalanceReport(const std::string& year,
    ledger::PUBLISHER_MONTH month, ledger::BalanceReportInfo* report_info) {
  std::map<std::string, braveledger_bat_helper::REPORT_BALANCE_ST>::const_iterator iter = 
    state_->monthly_balances_.find(GetBalanceReportName(year, month));
  DCHECK(iter != state_->monthly_balances_.end() && report_info);
  if (iter == state_->monthly_balances_.end() || !report_info) {
    return false;
  }

  report_info->opening_balance_ = iter->second.opening_balance_;
  report_info->closing_balance_ = iter->second.closing_balance_;
  report_info->grants_ = iter->second.grants_;
  report_info->earning_from_ads_ = iter->second.earning_from_ads_;
  report_info->auto_contribute_ = iter->second.auto_contribute_;
  report_info->recurring_donation_ = iter->second.recurring_donation_;
  report_info->one_time_donation_ = iter->second.one_time_donation_;

  return true;
}

void BatPublishers::saveState() {
  std::string data;
  braveledger_bat_helper::saveToJsonString(*state_, data);
  ledger_->SavePublisherState(data, this);
}

bool BatPublishers::loadState(const std::string& data) {
  braveledger_bat_helper::PUBLISHER_STATE_ST state;
  if (!braveledger_bat_helper::loadFromJson(state, data.c_str()))
    return false;

  state_.reset(new braveledger_bat_helper::PUBLISHER_STATE_ST(state));
  calcScoreConsts();
  return true;
}

void BatPublishers::OnPublisherStateSaved(ledger::Result result) {
  if (result != ledger::Result::OK) {
    LOG(ERROR) << "Could not save publisher state";
    // TODO - error handling
    return;
  }
  synopsisNormalizer();
}

std::vector<ledger::ContributionInfo> BatPublishers::GetRecurringDonationList() {
  std::vector<ledger::ContributionInfo> res;

  for (const auto & e : state_->recurring_donation_) {
    ledger::ContributionInfo info;
    info.publisher = e.first;
    info.value = e.second;
    res.push_back(info);
  }

  return res;
}

void BatPublishers::RefreshPublishersList(const std::string & pubs_list) {
  ledger_->SavePublishersList(pubs_list);
}

void BatPublishers::OnPublishersListSaved(ledger::Result result) {
  //TODO
}

}  // namespace braveledger_bat_publisher
